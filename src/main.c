#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

#define PORT 8080
#define BUFFER_SIZE 4096

void send_file(int client_socket, char *filename) {
    FILE *file = fopen(filename, "rb");

    if (file == NULL) {
        printf("❌ File not found: %s\n", filename);
        char *not_found = "HTTP/1.1 404 Not Found\nContent-Type: text/plain\n\n404 Not Found: File is missing!";
        send(client_socket, not_found, strlen(not_found), 0);
        return;
    }

    // 파일 크기
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 메모리 할당
    char *file_content = malloc(fsize + 1);
    fread(file_content, 1, fsize, file);
    file_content[fsize] = 0;
    fclose(file);

    // HTTP 헤더
    char header[BUFFER_SIZE];
    sprintf(header, "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: %ld\r\n"
            "\r\n", fsize);

    // 전송
    send(client_socket, header, strlen(header), 0);
    send(client_socket, file_content, fsize, 0);

    printf("🚀 Served File: %s (%ld bytes)\n", filename, fsize);

    // ★ 메모리 해제
    free(file_content);
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;
#endif
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // 소켓 생성
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        return 1;
    }

    // 주소 설정 (재사용)
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind
    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("Bind failed");
        return 1;
    }

    // Listen
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        return 1;
    }

    printf("\n🌊 Sea Web Server is running on http://localhost:%d\n", PORT);

    // infinite loop
    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        // 요청 읽기
        recv(new_socket, buffer, BUFFER_SIZE, 0);

        send_file(new_socket, "www/index.html");

        // 연결 종료
#ifdef _WIN32
        closesocket(new_socket);
#else
        close(new_socket);
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
