#include "common.h"
#include "server.h"
#include "file.h"

#ifdef _WIN32
unsigned __stdcall handle_client(void *arg);
#else
void *handle_client(void *arg);
#endif

void start_server() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return;
#endif

    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("\n🌊 Sea Web Server (Modularized) is running on port %d\n", PORT);

    while (1) {
        int new_socket;
        if ((new_socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        int *client_sock = malloc(sizeof(int));
        *client_sock = new_socket;

#ifdef _WIN32
        HANDLE hThread;
        unsigned threadID;
        hThread = (HANDLE) _beginthreadex(NULL, 0, handle_client, (void *) client_sock, 0, &threadID);
        CloseHandle(hThread);
#else
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void *) client_sock) < 0) {
            perror("Could not create thread");
            free(client_sock);
        } else {
            pthread_detach(thread_id);
        }
#endif
    }

    // (Unreachable code)
#ifdef _WIN32
    closesocket(server_fd);
    WSACleanup();
#else
    close(server_fd);
#endif
}

#ifdef _WIN32
unsigned __stdcall handle_client(void *arg) {
#else
    void *handle_client(void *arg) {
#endif
    int client_socket = *((int *) arg);
    free(arg);

    char buffer[BUFFER_SIZE] = {0};
#ifdef _WIN32
    int valread = recv(client_socket, buffer, BUFFER_SIZE, 0);
#else
    int valread = read(client_socket, buffer, BUFFER_SIZE);
#endif

    if (valread > 0) {
        char method[16], path[256], protocol[16];
        sscanf(buffer, "%s %s %s", method, path, protocol);

        char file_path[512];
        if (strcmp(path, "/") == 0) {
            sprintf(file_path, "www/index.html");
        } else {
            sprintf(file_path, "www%s", path);
        }

        send_file(client_socket, file_path);
    }

#ifdef _WIN32
    closesocket(client_socket);
    return 0;
#else
    close(client_socket);
    return NULL;
#endif
}
