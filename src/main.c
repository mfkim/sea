#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#endif

#define PORT 8080
#define BUFFER_SIZE 4096

const char *get_mime_type(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot) return "text/plain";
    if (strcmp(dot, ".html") == 0) return "text/html";
    if (strcmp(dot, ".css") == 0) return "text/css";
    if (strcmp(dot, ".png") == 0) return "image/png";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(dot, ".ico") == 0) return "image/x-icon";
    if (strcmp(dot, ".json") == 0) return "application/json";
    if (strcmp(dot, ".js") == 0) return "application/javascript";
    return "text/plain";
}

#ifdef _WIN32
unsigned __stdcall handle_client(void *arg) {
#else
    void *handle_client(void *arg) {
#endif
    int client_socket = *((int *) arg);
    free(arg); // 힙에 할당했던 메모리 해제

    char buffer[BUFFER_SIZE] = {0};

    // 요청 읽기
#ifdef _WIN32
    int valread = recv(client_socket, buffer, BUFFER_SIZE, 0);
#else
    int valread = read(client_socket, buffer, BUFFER_SIZE);
#endif

    if (valread > 0) {
        // 요청 분석 (Parsing)
        char method[16], path[256], protocol[16];
        sscanf(buffer, "%s %s %s", method, path, protocol);

        // 라우팅
        char file_path[512];
        if (strcmp(path, "/") == 0) {
            sprintf(file_path, "www/index.html");
        } else {
            sprintf(file_path, "www%s", path);
        }

        // --- 파일 전송 로직 ---
        FILE *file = fopen(file_path, "rb");
        if (file == NULL) {
            printf("❌ [%d] File not found: %s\n", client_socket, file_path);
            char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 Not Found";
            send(client_socket, not_found, strlen(not_found), 0);
        } else {
            fseek(file, 0, SEEK_END);
            long fsize = ftell(file);
            fseek(file, 0, SEEK_SET);

            char *file_content = malloc(fsize);
            if (file_content) {
                fread(file_content, 1, fsize, file);

                const char *mime_type = get_mime_type(file_path);
                char header[BUFFER_SIZE];
                sprintf(header, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", mime_type, fsize);

                send(client_socket, header, strlen(header), 0);
                send(client_socket, file_content, fsize, 0);

                free(file_content);
                printf("🚀 [%d] Served: %s (%ld bytes)\n", client_socket, file_path, fsize);
            }
            fclose(file);
        }
    }

    // 연결 종료
#ifdef _WIN32
    closesocket(client_socket);
    return 0;
#else
    close(client_socket);
    return NULL;
#endif
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;
#endif

    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("Bind failed");
        return 1;
    }
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        return 1;
    }

    printf("\n🌊 Sea Web Server (Multi-threaded) is running on port %d\n", PORT);

    while (1) {
        // 1. Accept
        int new_socket;
        if ((new_socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        // 2. 소켓 포인터 생성
        int *client_sock = malloc(sizeof(int));
        *client_sock = new_socket;

        // 3. 스레드 생성
#ifdef _WIN32
        HANDLE hThread;
        unsigned threadID;
        hThread = (HANDLE) _beginthreadex(NULL, 0, handle_client, (void *) client_sock, 0, &threadID);
        CloseHandle(hThread); // 스레드가 끝나면 리소스 해제
#else
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void *) client_sock) < 0) {
            perror("Could not create thread");
            free(client_sock);
            continue;
        }
        pthread_detach(thread_id);
#endif
    }

#ifdef _WIN32
    closesocket(server_fd);
    WSACleanup();
#else
    close(server_fd);
#endif
    return 0;
}
