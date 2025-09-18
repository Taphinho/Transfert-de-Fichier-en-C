#include "network.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
//#include <sys/select.h>
#ifdef _WIN32
    //#include <windows.h>
    #define usleep(x) Sleep((x)/1000)
#endif


#define BUFFER_SIZE 1024
#define MAX_CLIENTS 5
#define BASE_DIR "Base"

void send_file(FILE *file, int client_socket, long filesize, int number) {
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    int total_sent = 0;

    for (int i = 0; i < number; i++) {
        bytes_read = fread(buffer, 1, BUFFER_SIZE, file);
        if (bytes_read <= 0) break;

        if (send(client_socket, buffer, bytes_read, 0) < 0) {
            perror("send failed");
            break;
        }
        total_sent += bytes_read;
    }
}

void handle_get(int client_socket, const char *filename) {
    char filepath[BUFFER_SIZE];
    snprintf(filepath, sizeof(filepath), "%s%s%s", BASE_DIR, DIR_SEPARATOR, filename);

    FILE *file = fopen(filepath, "rb");
    if (!file) {
        send(client_socket, "ERROR:File not found", 20, 0);
        return;
    }

    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    int number = (filesize + BUFFER_SIZE - 1) / BUFFER_SIZE;
    char response[100];
    sprintf(response, "OK:%ld:%d", filesize, number);
    send(client_socket, response, strlen(response), 0);

    // Petite pause pour la synchronisation
    usleep(10000);

    send_file(file, client_socket, filesize, number);
    fclose(file);
}

void handle_write(int client_socket, const char *filename, long filesize, int number) {
    char filepath[BUFFER_SIZE];
    snprintf(filepath, sizeof(filepath), "%s%s%s", BASE_DIR, DIR_SEPARATOR, filename);

    FILE *file = fopen(filepath, "wb");
    if (!file) {
        perror("fopen failed");
        send(client_socket, "ERROR:Cannot open file", 23, 0);
        return;
    }

    char buffer[BUFFER_SIZE];
    int total_received = 0;

    send(client_socket, "READY", 5, 0);

    for (int i = 0; i < number; i++) {
        int bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0) {
            perror("recv failed");
            break;
        }

        fwrite(buffer, 1, bytes, file);
        total_received += bytes;
        printf("Paquet %d reçu (%d/%ld octets)\n", i + 1, total_received, filesize);
    }

    fclose(file);
    printf("Fichier %s reçu avec succès\n", filename);
}

void handle_list(int client_socket) {
    char response[BUFFER_SIZE] = {0};
    char *ptr = response;
    ptr += sprintf(ptr, "LIST:");

#ifdef _WIN32
    WIN32_FIND_DATA findFileData;
    HANDLE hFind;
    char searchPath[MAX_PATH];
    snprintf(searchPath, sizeof(searchPath), "%s\\*", BASE_DIR);

    hFind = FindFirstFile(searchPath, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        send(client_socket, "ERROR:Cannot open directory", 28, 0);
        return;
    }

    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            ptr += sprintf(ptr, "%s ", findFileData.cFileName);
        }
    } while (FindNextFile(hFind, &findFileData) != 0);
    FindClose(hFind);
#else
    DIR *dir;
    struct dirent *entry;

    dir = opendir(BASE_DIR);
    if (!dir) {
        perror("opendir failed");
        send(client_socket, "ERROR:Cannot open directory", 28, 0);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            ptr += sprintf(ptr, "%s ", entry->d_name);
        }
    }
    closedir(dir);
#endif

    send(client_socket, response, strlen(response), 0);
}


int main(int argc, char *argv[]) {
    int PORT = (argc == 2) ? atoi(argv[1]) : 2121;
    if (PORT <= 1023) {
        printf("Port réservé! Utilisation du port 2121\n");
        PORT = 2121;
    }

    initNetwork();

    int server_socket, client_socket, max_sd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    fd_set readfds, masterfds;

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Serveur en écoute sur le port %d...\n", PORT);

    FD_ZERO(&masterfds);
    FD_SET(server_socket, &masterfds);
    max_sd = server_socket;

    while (1) {
        readfds = masterfds;
        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) == -1) {
            perror("select failed");
            exit(EXIT_FAILURE);
        }

        for (int sock = 0; sock <= max_sd; sock++) {
            if (FD_ISSET(sock, &readfds)) {
                if (sock == server_socket) {
                    if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len)) < 0) {
                        perror("accept failed");
                        continue;
                    }
                    FD_SET(client_socket, &masterfds);
                    if (client_socket > max_sd) {
                        max_sd = client_socket;
                    }
                    printf("Nouveau client connecté: %d\n", client_socket);
                } else {
                    char buffer[BUFFER_SIZE] = {0};
                    int valread = recv(sock, buffer, BUFFER_SIZE, 0);

                    if (valread <= 0) {
                        printf("Client déconnecté: %d\n", sock);
                        close(sock);
                        FD_CLR(sock, &masterfds);
                        continue;
                    }

                    printf("Commande reçue de %d: %s\n", sock, buffer);

                    if (strncmp(buffer, "GET:", 4) == 0) {
                        handle_get(sock, buffer + 4);
                    }
                    else if (strncmp(buffer, "WRITE:", 6) == 0) {
                        char *ptr = buffer + 6;
                        char *filename = strtok(ptr, ":");
                        char *size_str = strtok(NULL, ":");
                        char *number_str = strtok(NULL, ":");
                        if (!filename || !size_str || !number_str) {
                            send(sock, "ERROR:Invalid request format", 27, 0);
                            continue;
                        }
                        long filesize = atol(size_str);
                        int number = atoi(number_str);

                        handle_write(sock, filename, filesize, number);
                    }
                    else if (strncmp(buffer, "LIST", 4) == 0) {
                        handle_list(sock);
                    }
                    else {
                        send(sock, "ERROR:Invalid command", 22, 0);
                    }
                }
            }
        }
    }

    closesocket(server_socket);
    cleanNetwork();
    return 0;
}

