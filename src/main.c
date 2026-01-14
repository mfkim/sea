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

// ----------------------------------------------------------
// 파일 확장자에 따른 MIME Type 반환
// ----------------------------------------------------------
const char *get_mime_type(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot) return "text/plain";

    if (strcmp(dot, ".html") == 0) return "text/html";
    if (strcmp(dot, ".css") == 0) return "text/css";
    if (strcmp(dot, ".js") == 0) return "application/javascript";
    if (strcmp(dot, ".png") == 0) return "image/png";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(dot, ".gif") == 0) return "image/gif";
    if (strcmp(dot, ".ico") == 0) return "image/x-icon";
    if (strcmp(dot, ".json") == 0) return "application/json";

    return "text/plain";
}

// ----------------------------------------------------------
// 클라이언트에게 파일 전송
// ----------------------------------------------------------
void send_file(int client_socket, char *filename) {
    FILE *file = fopen(filename, "rb");

    if (file == NULL) {
        printf("❌ File not found: %s\n", filename);
        char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 Not Found: File is missing!";
        send(client_socket, not_found, strlen(not_found), 0);
        return;
    }

    // 1. 파일 크기 측정
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 2. 파일 내용 읽기
    char *file_content = malloc(fsize);
    if (file_content) {
        fread(file_content, 1, fsize, file);
    }
    fclose(file);

    // 3. MIME Type 자동 감지
    const char *mime_type = get_mime_type(filename);

    // 4. HTTP 헤더 작성
    char header[BUFFER_SIZE];
    sprintf(header, "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n"
            "\r\n", mime_type, fsize);

    // 5. 전송 (헤더 -> 바디)
    send(client_socket, header, strlen(header), 0);
    if (file_content) {
        send(client_socket, file_content, fsize, 0);
        free(file_content); // ★ 메모리 해제
    }

    printf("🚀 Served: %s | Type: %s | Size: %ld bytes\n", filename, mime_type, fsize);
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

    // 소켓 생성 및 설정
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

    printf("\n🌊 Sea Web Server is running on http://localhost:%d\n", PORT);

    // [무한 루프]
    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        // 요청 읽기
        int valread = recv(new_socket, buffer, BUFFER_SIZE, 0);
        if (valread > 0) {
            // ----------------------------------------------------------
            // [요청 분석] GET /index.html HTTP/1.1 형태 파싱
            // ----------------------------------------------------------
            char method[16], path[256], protocol[16];
            sscanf(buffer, "%s %s %s", method, path, protocol);

            printf("\n📩 Request: %s %s\n", method, path);

            // [라우팅] 경로 매핑
            char file_path[512];

            if (strcmp(path, "/") == 0) {
                sprintf(file_path, "www/index.html");
            } else {
                sprintf(file_path, "www%s", path);
            }

            send_file(new_socket, file_path);
        }

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
