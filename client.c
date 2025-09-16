// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <stdint.h>
#include <endian.h>   // htobe64

#define PORT 1717
#define BUFFER_SIZE 4096

// lee tamaño de archivo
int64_t get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0) return (int64_t)st.st_size;
    return -1;
}

// enviar todo (similar a send, pero en bucle)
ssize_t send_all(int sock, const void *buf, size_t len) {
    size_t total = 0;
    const char *p = buf;
    while (total < len) {
        ssize_t n = send(sock, p + total, len - total, 0);
        if (n <= 0) return n;
        total += n;
    }
    return (ssize_t)total;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <ip_servidor> <archivo_imagen>\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    const char *filepath = argv[2];

    // obtener solo el nombre base (sin path)
    const char *base = strrchr(filepath, '/');
    if (base) base++; else base = filepath;
    uint32_t name_len = (uint32_t)strlen(base);

    int64_t filesize = get_file_size(filepath);
    if (filesize < 0) {
        perror("No se pudo obtener tamaño del archivo");
        return 1;
    }

    FILE *f = fopen(filepath, "rb");
    if (!f) { perror("No se pudo abrir archivo"); return 1; }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); fclose(f); return 1; }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, server_ip, &addr.sin_addr) <= 0) {
        perror("Dirección inválida");
        close(sock); fclose(f); return 1;
    }

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Conexión fallida");
        close(sock); fclose(f); return 1;
    }

    // 1) enviar name_len (network order)
    uint32_t name_len_net = htonl(name_len);
    if (send_all(sock, &name_len_net, sizeof(name_len_net)) <= 0) {
        perror("send name_len");
        close(sock); fclose(f); return 1;
    }

    // 2) enviar name (N bytes, NO terminador)
    if (send_all(sock, base, name_len) <= 0) {
        perror("send name");
        close(sock); fclose(f); return 1;
    }

    // 3) enviar filesize (int64_t en big-endian)
    int64_t filesize_net = htobe64((int64_t)filesize);
    if (send_all(sock, &filesize_net, sizeof(filesize_net)) <= 0) {
        perror("send filesize");
        close(sock); fclose(f); return 1;
    }

    // 4) enviar contenido en bloques
    char buffer[BUFFER_SIZE];
    int64_t sent = 0;
    while (!feof(f)) {
        size_t n = fread(buffer, 1, sizeof(buffer), f);
        if (n > 0) {
            if (send_all(sock, buffer, n) <= 0) {
                perror("send file chunk");
                close(sock); fclose(f); return 1;
            }
            sent += n;
        }
    }
    fclose(f);

    printf("Archivo enviado (%lld bytes)\n", (long long)sent);

    // leer respuesta del servidor (una línea)
    int r = recv(sock, buffer, sizeof(buffer)-1, 0);
    if (r > 0) {
        buffer[r] = '\0';
        printf("Respuesta del servidor: %s\n", buffer);
    }

    close(sock);
    return 0;
}
