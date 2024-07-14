#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 8800
#define DEBUG 1
#define BUFFER_SIZE 1024
int max_clients;

struct userNode* users;
pthread_mutex_t globalLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sendLock = PTHREAD_MUTEX_INITIALIZER;

// Structure to represent each user connected to the server
struct userNode{
    char* username;
    int client_fd;
    time_t last_activity;
    
    struct userNode* next;
};

struct userNode* userListHead;
int currentLength = 0;

struct passingArg{
    char* username;
    int client_fd;
};

// Function to reset a string to empty
void reset_string(char *string)
{
    memset(string, 0, strlen(string)+1);
}

// Function to remove a client from the user list
void removeClient(int client_fd) {
    pthread_mutex_lock(&globalLock);
    struct userNode* temp = userListHead;
    struct userNode* prev = NULL;
    while (temp != NULL) {
        if (temp->client_fd == client_fd) {
            if (prev == NULL) {
                userListHead = temp->next;
            } else {
                prev->next = temp->next;
            }
            free(temp->username);
            free(temp);
            break;
        }
        prev = temp;
        temp = temp->next;
    }
    currentLength--;
    pthread_mutex_unlock(&globalLock);
}

// Function to send the list of users to a client
void sendUserList(int client_fd) {
    struct userNode* temp = userListHead;
    char userList[BUFFER_SIZE];
    memset(userList, 0, BUFFER_SIZE);
    strcat(userList, "List of users:\n");
    while (temp != NULL) {
        strcat(userList, temp->username);
        if(temp->client_fd == client_fd) {
            strcat(userList, " (You)");
        }
        strcat(userList, "\n");
        temp = temp->next;
    }
    printf("Sending list to client %d\n", client_fd);  
    printf("user list: %s\n", userList); 
    send(client_fd, userList, strlen(userList) + 1, 0);
}

// Function to broadcast a message to all clients except the sender
void broadcast(int client_fd, char* message) {
    struct userNode* temp = userListHead;
    while (temp != NULL) {
        if (temp->client_fd != client_fd) {
            send(temp->client_fd, message, strlen(message) + 1, 0);
        }
        temp = temp->next;
    }
}

void* welcome_client(void* arg) {
    struct passingArg* obj = (struct passingArg*)arg;
    int client_fd = obj->client_fd;

    char welcomeMessage[] = "Enter your username: ";
    send(client_fd, welcomeMessage, strlen(welcomeMessage)+1, 0);
    
    char username[100];
    read(client_fd, username, 100);

    pthread_mutex_lock(&globalLock);

    // Check if the server has reached the maximum number of clients
    if(currentLength == max_clients) {
        char maxClientsMessage[] = "Maximum number of clients reached, please try again later";
        send(client_fd, maxClientsMessage, strlen(maxClientsMessage) + 1, 0);
        close(client_fd);
        pthread_mutex_unlock(&globalLock);
        pthread_exit(NULL);
    }

    // Check if the username is already taken
    struct userNode* checker = userListHead;
    while(checker != NULL){
        if(strcmp(checker->username, username)==0){
            char usernameTaken[] = "Username already taken! Please try again with a different username.";
            send(client_fd, usernameTaken, strlen(usernameTaken) + 1, 0);
            close(client_fd);
            pthread_mutex_unlock(&globalLock);
            pthread_exit(NULL);
        }else{
            checker = checker->next;
        }   
    }

    obj->username = strdup(username);

    char personalWelcome[120];
    snprintf(personalWelcome, sizeof(personalWelcome), "Welcome %s!", username);
    send(client_fd, personalWelcome, sizeof(personalWelcome), 0);

    // Add the user to the user list
    struct userNode* newUser = (struct userNode*)malloc(sizeof(struct userNode));
    newUser->username = strdup(username);
    newUser->client_fd = client_fd;
    newUser->next = NULL;
    newUser->last_activity = time(NULL);

    if(userListHead == NULL) {
        userListHead = newUser;
    } else {
        struct userNode* temp = userListHead;
        while(temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = newUser;
    }
    currentLength++;

    // ***************************

    printf("User list: \n");
    struct userNode* temp = userListHead;
    while (temp != NULL) {
        printf("%s\n", temp->username);
        temp = temp->next;
    }

    char newUserMessage[200];
    snprintf(newUserMessage, sizeof(newUserMessage), "Client %s joined the chatroom", username);
    broadcast(client_fd, newUserMessage);

    sendUserList(client_fd);

    if(DEBUG)printf("Sent list of users\n");

    reset_string(username);

    pthread_mutex_unlock(&globalLock);

    char buffer[BUFFER_SIZE];
    while (1) {
        reset_string(buffer);
        int readcode = read(client_fd, buffer, sizeof(buffer));
        if (readcode <= 0) {
            printf("%s disconnected\n", obj->username);

            removeClient(client_fd);
            char message[200];
            snprintf(message, sizeof(message), "Client %s left the chatroom", obj->username);
            broadcast(client_fd, message);
            close(client_fd);
            break;

        }
        if (DEBUG) printf("Client %d: %s\n", client_fd, buffer);
        
        pthread_mutex_lock(&globalLock);
        struct userNode* temp = userListHead;
        while (temp != NULL) {
            if (temp->client_fd == client_fd) {
                temp->last_activity = time(NULL);
                break;
            }
            temp = temp->next;
        }
        pthread_mutex_unlock(&globalLock);

        if (strncmp(buffer, "\\list", 5) == 0) {
            sendUserList(client_fd);
        } else if (strncmp(buffer, "\\bye", 4) == 0) {
            send(client_fd, "Goodbye!", sizeof("Goodbye!"), 0);
            // Remove the client from the user list
            removeClient(client_fd);
            char message[200];
            snprintf(message, sizeof(message), "Client %s left the chatroom", obj->username);
            broadcast(client_fd, message);
            close(client_fd);
            break;
        } else {
            char message[1050];
            snprintf(message, sizeof(message), "%s: %s", obj->username, buffer);
            broadcast(client_fd, message);
        }
    }

    pthread_exit(NULL);
}

// Thread function to check for timeouts and disconnect inactive clients
void* checkTimeout(void* arg){
    int timeout = *((int*)arg);
    pthread_mutex_t timeLock = PTHREAD_MUTEX_INITIALIZER;
    while(1) {
        pthread_mutex_lock(&timeLock);
        struct userNode* temp = userListHead;
        while(temp != NULL) {
            if (time(NULL) - temp->last_activity > timeout) {
                printf("%s timed out\n", temp->username);
                int toCloseFd = temp->client_fd;
                char* toCloseUsername = strdup(temp->username);
                temp = temp->next;
                removeClient(toCloseFd);
                char message[200];
                snprintf(message, sizeof(message), "Client %s left the chatroom (timed out)", toCloseUsername);
                send(toCloseFd, "You have been timed out. Press any character + ENTER to exit", sizeof("You have been time out. Press any character + ENTER to exit"), 0);
                broadcast(toCloseFd, message);
                close(toCloseFd);
            }else{
                temp = temp->next;
            }
        }
        pthread_mutex_unlock(&timeLock);
        sleep(1);
    }
    pthread_exit(NULL);
}


int main(int argc, char** argv) {
    if(argc!=4) {
        printf("Wrong Usage: %s <port> <max_clients> <timeout>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    userListHead = NULL;

    int port = atoi(argv[1]);
    max_clients = atoi(argv[2]);
    int timeout = atoi(argv[3]);

    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if((server_fd = socket(AF_INET, SOCK_STREAM, 0))==0){
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, max_clients) < 0) {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", port);

    pthread_t timeoutCheck;
    pthread_create(&timeoutCheck, NULL, (void *)checkTimeout, (void*)&timeout);

    while(1) {
        if(DEBUG)printf("Waiting for a new client\n");
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Acceptance failed");
            exit(EXIT_FAILURE);
        }

        if(DEBUG)printf("Client fd: %d\n", client_fd);

        pthread_t thread;

        struct passingArg* arg = (struct passingArg *)malloc(sizeof(struct passingArg));
        arg->client_fd = client_fd;
        arg->username = NULL;

        printf("Creating new thread\n");
        if(pthread_create(&thread, NULL, (void *)welcome_client, (void *)arg) != 0) {
            perror("Thread creation failed");
            exit(EXIT_FAILURE);
        }
        if(DEBUG)printf("Thread returns\n");

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(address.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("Client IP address: %s\n", client_ip);
    }
    
    close(server_fd);

    return 0;
}
