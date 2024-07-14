#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>

int PORT;
#define BUFFER_SIZE 2000
#define MAX_CLIENTS 10

char* ROOT_DIRECTORY;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg);
void handle_get_request(int client_sock, char *path);
void handle_post_request(int client_sock, char *data);
void evaluate_counts(char *sentence, int client_sock);

// Function to count characters, words, and sentences in a string
void evaluate_counts(char *sentence,int client_sock)
{
    int SENT_SIZE = strlen(sentence)+1;
    int characters = 0, words = 0, sentences = 0;
    int in_word = 0;

    for (int i = 0; i < SENT_SIZE; i++) {
        if (sentence[i] == '\0')
            break; 

        if (isprint(sentence[i]))
            characters++; 

        if (isspace(sentence[i])) {
            // If we were in a word, increment word count and mark that we are no longer in a word
            if (in_word) {
                words++;
                in_word = 0;
            }
        } else {
            in_word = 1;
        }

        if(sentence[i] == '.' || sentence[i] == '?' || sentence[i] == '!'){
            sentences++;
        }
    }

    if(in_word){
        words++;
    }

    char result[200];
    sprintf(result, "Number of characters: %d\r\nNumber of words: %d\r\nNumber of sentences: %d\r\n", characters, words, sentences);
    printf("%s", result);

    // Prepare HTTP response headers
    char *content_type = "text/plain";
    char header[BUFFER_SIZE];
    int content_length = strlen(result) + 1; 
    sprintf(header, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n Content-Length: %d\r\n\r\n", content_type, content_length);
    
    // Send HTTP response
    send(client_sock, header, strlen(header)+1, 0);  
    send(client_sock, result, strlen(result)+1, 0); 

}

// Function to extract substring between delimiters
char* find_enclosed_substring(const char* buffer) {
    const char* start_delimiter = "%**%";
    const char* end_delimiter = "%**%";
    const char* start_ptr = strstr(buffer, start_delimiter); 
    if (start_ptr == NULL)
        return NULL;

    start_ptr += strlen(start_delimiter); 

    const char* end_ptr = strstr(start_ptr, end_delimiter); 
    if (end_ptr == NULL)
        return NULL; 
    size_t length = end_ptr - start_ptr;

    char* enclosed_substring = malloc(length + 1);
    if (enclosed_substring == NULL)
        return NULL; 

    strncpy(enclosed_substring, start_ptr, length);
    enclosed_substring[length] = '\0';

    return enclosed_substring;
}

// Function to handle client requests
void *handle_client(void *arg) {
    int client_sock = *((int *)arg);

    pthread_mutex_lock(&mutex);
    char buffer[BUFFER_SIZE];
    
    if (recv(client_sock, buffer, BUFFER_SIZE, 0) <= 0) {
        perror("recv failed");
        close(client_sock);
        return NULL;
    }
    
    char method[10], path[100];
    sscanf(buffer, "%s %s", method, path);
    printf("Going to handle %s request\n", method);

    // Seperately handling GET or POST request
    if (strcmp(method, "GET") == 0) 
    {
        printf("Entering get request handling\n");
        handle_get_request(client_sock, path);

    } 
    else if (strcmp(method, "POST") == 0) {                                                                                                                                                                                                                                                                                                                                                                            
        printf("Entering post request handling\n");
        // need to traverse the buffer to the substring %**% and store the string between them
        char* data = find_enclosed_substring(buffer);
        printf("Data: \n%s", data); 

        evaluate_counts(data,client_sock);
    }
    
    pthread_mutex_unlock(&mutex);

    close(client_sock);
    return NULL;
}

// Function to handle GET requests
void handle_get_request(int client_sock, char *path) {
    char full_path[256];
    FILE *file;

    if (strcmp(path, "/") == 0) {
        strcpy(full_path, ROOT_DIRECTORY);
        strcat(full_path, "/index.html");
    } else {
        strcpy(full_path, ROOT_DIRECTORY);
        strcat(full_path, path);
    }

    if ((file = fopen(full_path, "rb")) == NULL) {
        strcpy(full_path, ROOT_DIRECTORY);
        strcat(full_path, "/404.html");
        file = fopen(full_path, "rb");
    }

    // Determine content type based on file extension
    char *content_type;
    char *file_extension = strrchr(full_path, '.');

    if (file_extension != NULL) {
        if (strcmp(file_extension, ".html") == 0 || strcmp(file_extension, ".htm") == 0) {
            content_type = "text/html";
        } else if (strcmp(file_extension, ".css") == 0) {
            content_type = "text/css";
        } else if (strcmp(file_extension, ".js") == 0) {
            content_type = "application/javascript";
        } else if (strcmp(file_extension, ".jpg") == 0 || strcmp(file_extension, ".jpeg") == 0) {
            content_type = "image/jpeg";
        } else if (strcmp(file_extension, ".png") == 0) {
            content_type = "image/png";
        } else {
            content_type = "text/plain";
        }
    } else {
        content_type = "text/plain";
    }

    // Send HTTP response with appropriate content type
    char response[BUFFER_SIZE];
    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", content_type);
    send(client_sock, response, strlen(response), 0);

    // Send file content
    while (!feof(file)) {
        size_t bytes_read = fread(response, 1, BUFFER_SIZE, file);
        send(client_sock, response, bytes_read, 0);
    }

    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <web_root>\n", argv[0]);
        return EXIT_FAILURE;
    }

    PORT = atoi(argv[1]);
    ROOT_DIRECTORY = (char*)(malloc(strlen(argv[2])+1));
    ROOT_DIRECTORY = argv[2];

    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    int cl_addrlen = sizeof(client_addr);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&cl_addrlen)) < 0) {
            perror("Acceptance failed");
            exit(EXIT_FAILURE);
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, &client_fd) != 0) {
            perror("pthread_create failed");
            exit(EXIT_FAILURE);
        }
    }

    close(server_fd);
    return 0;
}