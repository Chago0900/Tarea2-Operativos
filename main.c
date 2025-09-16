// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <time.h>
#include <stdint.h>
#include <endian.h>   // be64toh
#include "clasificador.h"

#define DEFAULT_PORT 1717
#define LOG_FILE "/var/log/imageserver.log"
#define BUFFER_SIZE 4096

void log_event(const char *client_ip, const char *filename, const char *status) {
    FILE *f = fopen(LOG_FILE, "a");
    if (!f) return;
    time_t now = time(NULL);
    fprintf(f, "[%s] Cliente: %s, Archivo: %s, Estado: %s\n",
            ctime(&now), client_ip, filename, status);
    fclose(f);
}

// recibe exactamente len bytes (útil para encabezados)
ssize_t recv_all(int sock, void *buf, size_t len) {
    size_t total = 0;
    char *p = buf;
    while (total < len) {
        ssize_t n = recv(sock, p + total, len - total, 0);
        if (n <= 0) return n;
        total += n;
    }
    return (ssize_t)total;
}

int ensure_dir_exists(const char *path) {
    struct stat st;
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0755) != 0) return -1;
    }
    return 0;
}

int main() {
    int port = DEFAULT_PORT;
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // crear dirs de salida si no existen
    ensure_dir_exists("rojas");
    ensure_dir_exists("verdes");
    ensure_dir_exists("azules");

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind"); return 1;
    }
    if (listen(server_fd, 5) < 0) {
        perror("listen"); return 1;
    }

    printf("Servidor escuchando en puerto %d...\n", port);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0) { perror("accept"); continue; }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

        // -------- recibir name_len
        uint32_t name_len_net;
        if (recv_all(client_fd, &name_len_net, sizeof(name_len_net)) <= 0) {
            close(client_fd); continue;
        }
        uint32_t name_len = ntohl(name_len_net);
        if (name_len == 0 || name_len > 1024) { close(client_fd); continue; }

        // recibir nombre (exactamente name_len bytes)
        char namebuf[2048];
        if (recv_all(client_fd, namebuf, name_len) <= 0) {
            close(client_fd); continue;
        }
        namebuf[name_len] = '\0'; // terminar

        // recibir filesize (int64_t big-endian)
        int64_t filesize_net;
        if (recv_all(client_fd, &filesize_net, sizeof(filesize_net)) <= 0) {
            close(client_fd); continue;
        }
        int64_t filesize = be64toh(filesize_net);
        if (filesize < 0) { close(client_fd); continue; }

        printf("Cliente %s envió archivo: %s (%lld bytes)\n",
               client_ip, namebuf, (long long)filesize);

        // abrir archivo local para escribir
        FILE *f = fopen(namebuf, "wb");
        if (!f) {
            perror("fopen");
            close(client_fd);
            continue;
        }

        // recibir content exactamente filesize bytes
        char buffer[BUFFER_SIZE];
        int64_t remaining = filesize;
        int ok = 1;
        while (remaining > 0) {
            ssize_t toread = remaining > sizeof(buffer) ? sizeof(buffer) : (size_t)remaining;
            ssize_t r = recv_all(client_fd, buffer, toread);
            if (r <= 0) { ok = 0; break; }
            fwrite(buffer, 1, r, f);
            remaining -= r;
        }
        fclose(f);

        if (!ok || remaining != 0) {
            // archivo incompleto - eliminar y reportar
            unlink(namebuf);
            send(client_fd, "Error transfiriendo archivo\n", 27, 0);
            log_event(client_ip, namebuf, "TRANSFER ERROR");
            close(client_fd);
            continue;
        }

        // clasificar (usa tu función existente)
        if (classify_image(namebuf, namebuf, "rojas", "verdes", "azules") == 0) {
            send(client_fd, "Imagen clasificada con éxito\n", 30, 0);
            log_event(client_ip, namebuf, "OK");
        } else {
            send(client_fd, "Error clasificando imagen\n", 27, 0);
            log_event(client_ip, namebuf, "ERROR");
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}
