#include "common.h"
#include "file.h"
#include "http.h"

void send_file(int client_socket, char *filename) {
    FILE *file = fopen(filename, "rb");

    if (file == NULL) {
        printf("❌ [%d] File not found: %s\n", client_socket, filename);
        char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 Not Found";
        send(client_socket, not_found, strlen(not_found), 0);
        return;
    }

    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *file_content = malloc(fsize);
    if (file_content) {
        fread(file_content, 1, fsize, file);

        const char *mime_type = get_mime_type(filename);
        char header[BUFFER_SIZE];
        sprintf(header, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", mime_type, fsize);

        send(client_socket, header, strlen(header), 0);
        send(client_socket, file_content, fsize, 0);

        free(file_content);
        printf("🚀 [%d] Served: %s (%ld bytes)\n", client_socket, filename, fsize);
    }
    fclose(file);
}
