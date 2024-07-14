#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 8800

void send_file(int client_fd, const char* filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(client_fd, buffer, bytes_read, 0) != bytes_read) {
            perror("Error sending file");
            exit(EXIT_FAILURE);
        }
    }

    fclose(file);
}

int main(int argc, char** argv) {
    if (argc != 4) {
        printf("Usage: %s <port> <root_dir> <max_streams>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    char *root_dir = argv[2];
    int max_streams = atoi(argv[3]);

    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
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

    if (listen(server_fd, max_streams) < 0) {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", port);

    while(1) {
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Acceptance failed");
            exit(EXIT_FAILURE);
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(address.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("Client IP address: %s\n", client_ip);

        char request[2];
        // corresponding to the first send (song id) by the client 
        read(client_fd, request, sizeof(request));
        // request is of the form "<single_digit>\n<ip_address>", get the first character
        char first_char = request[0];

        char filename[100]; // Assuming filenames like "1.mp3", "2.mp3", ..., "9.mp3"

        char str1[2] = {first_char , '\0'};
        char fin_request[5] = "";
        strcpy(fin_request,str1);

        strcat(filename, root_dir);
        strcat(filename, "/");
        strcat(filename, fin_request);
        strcat(filename, ".mp3");

        printf("Requested path: %s\n", filename);

        printf("Sending file: %s\n", filename);
        // corresponding to the recv request by the client, expects server to send the mp3 file.
        send_file(client_fd, filename);

        close(client_fd);
    }
    
    close(server_fd);

    return 0;
}
