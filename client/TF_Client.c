#include "network1.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define SERVER_IP "172.20.112.1"
//#define SERVER_IP "127.0.0.1"

void receive_file(int sock, const char *filename, long filesize, int number) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("fopen failed");
        return;
    }

    char buffer[BUFFER_SIZE];
    int total_received = 0;

    for (int i = 0; i < number; i++) {
        int bytes = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0) break;

        fwrite(buffer, 1, bytes, file);
        total_received += bytes;
        printf("\rReçu: %d/%d paquets (%.1f%%)",
              i + 1, number, (float)(i + 1)/number*100);
        fflush(stdout);
    }

    fclose(file);
    printf("\nTransfert terminé\n");
}

int main(int argc, char *argv[]) {
    int PORT = (argc == 2) ? atoi(argv[1]) : 2121;
    if (PORT <= 1023) {
        printf("Port réservé! Utilisation du port 2121\n");
        PORT = 2121;
    }

    initNetwork();

    int sock;
    struct sockaddr_in server_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        return -1;
    }

    printf("Connecté au serveur %s:%d\n", SERVER_IP, PORT);

    while (1) {
        printf("\nCommandes:\n");
        printf("  GET <fichier>\n");
        printf("  WRITE <fichier>\n");
        printf("  LIST\n");
        printf("  EXIT\n");
        printf("> ");

        char buffer[BUFFER_SIZE];
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

        if (strcasecmp(buffer, "EXIT") == 0) {
            break;
        }
        else if (strncasecmp(buffer, "GET ", 4) == 0) {
            char filename[256];
            sscanf(buffer + 4, "%255s", filename);

            char request[512];
            snprintf(request, sizeof(request), "GET:%s", filename);
            send(sock, request, strlen(request), 0);

            char response[BUFFER_SIZE] = {0};
            recv(sock, response, BUFFER_SIZE, 0);

            if (strncmp(response, "OK:", 3) == 0) {
                long filesize;
                int number;
                sscanf(response + 3, "%ld:%d", &filesize, &number);
                printf("Téléchargement %s (%ld octets en %d paquets)...\n", filename, filesize, number);
                receive_file(sock, filename, filesize, number);
            } else {
                printf("Erreur: %s\n", response);
            }
        }
        else if (strncasecmp(buffer, "WRITE ", 6) == 0) {
            char filename[256];
            sscanf(buffer + 6, "%255s", filename);

            FILE *file = fopen(filename, "rb");
            if (!file) {
                printf("Fichier introuvable\n");
                continue;
            }

            fseek(file, 0, SEEK_END);
            long filesize = ftell(file);
            fseek(file, 0, SEEK_SET);

            int number = (filesize + BUFFER_SIZE - 1) / BUFFER_SIZE;
            char request[512];
            snprintf(request, sizeof(request), "WRITE:%s:%ld:%d", filename, filesize, number);
            send(sock, request, strlen(request), 0);

            char response[BUFFER_SIZE] = {0};
            recv(sock, response, BUFFER_SIZE, 0);

            if (strcmp(response, "READY") == 0) {
                printf("Envoi %s (%ld octets en %d paquets)...\n", filename, filesize, number);

                char file_buffer[BUFFER_SIZE];
                int total_sent = 0;

                for (int i = 0; i < number; i++) {
                    size_t bytes = fread(file_buffer, 1, BUFFER_SIZE, file);
                    if (bytes <= 0) break;

                    send(sock, file_buffer, bytes, 0);
                    total_sent += bytes;
                    printf("\rEnvoyé: %d/%d paquets (%.1f%%)",
                          i + 1, number, (float)(i + 1)/number*100);
                    fflush(stdout);
                }
                printf("\n");
            } else {
                printf("Erreur serveur: %s\n", response);
            }

            fclose(file);
        }
        else if (strncasecmp(buffer, "LIST", 4) == 0) {
            send(sock, "LIST", 4, 0);

            char response[BUFFER_SIZE] = {0};
            recv(sock, response, BUFFER_SIZE, 0);

            if (strncmp(response, "LIST:", 5) == 0) {
                printf("Fichiers disponibles:\n%s\n", response + 5);
            } else {
                printf("Erreur: %s\n", response);
            }
        }
        else {
            printf("Commande invalide\n");
        }
    }

    close(sock);
    cleanNetwork();
    return 0;
}

