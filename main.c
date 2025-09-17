#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <time.h>
#include <stdint.h>
#include <endian.h>
#include "clasificador.h"
#include "histogram.h"

#define DEFAULT_PORT 1717
#define LOG_FILE "/var/log/imageserver.log"
#define BUFFER_SIZE 4096
#define DIR_ROJAS "/var/lib/imageserver/rojas"
#define DIR_VERDES "/var/lib/imageserver/verdes"
#define DIR_AZULES "/var/lib/imageserver/azules"
#define DIR_FILTRADO "/var/lib/imageserver/filtrado"


void log_event(const char *client_ip, const char *filename, const char *status) {
    FILE *f = fopen(LOG_FILE, "a");
    if (!f) return;
    time_t now = time(NULL);
    char *time_str = ctime(&now);
    time_str[strlen(time_str)-1] = '\0'; // Remove newline
    fprintf(f, "[%s] Cliente: %s, Archivo: %s, Estado: %s\n",
            time_str, client_ip, filename, status);
    fclose(f);
}

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

void generate_histogram_filename(const char *input, char *output, size_t output_size) {
    char *dot = strrchr(input, '.');
    if (dot) {
        size_t base_len = dot - input;
        snprintf(output, output_size, "%s/%.*s_equalized%s", 
                DIR_FILTRADO, (int)base_len, input, dot);
    } else {
        snprintf(output, output_size, "%s/%s_equalized.png", DIR_FILTRADO, input);
    }
}

int main() {
    int port = DEFAULT_PORT;
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Create output directories
    ensure_dir_exists(DIR_ROJAS);
    ensure_dir_exists(DIR_VERDES);
    ensure_dir_exists(DIR_AZULES);
    ensure_dir_exists(DIR_FILTRADO);

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

    printf("Servidor de procesamiento de imágenes escuchando en puerto %d...\n", port);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0) { perror("accept"); continue; }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

        // Receive name_len
        uint32_t name_len_net;
        if (recv_all(client_fd, &name_len_net, sizeof(name_len_net)) <= 0) {
            close(client_fd); continue;
        }
        uint32_t name_len = ntohl(name_len_net);
        if (name_len == 0 || name_len > 1024) { close(client_fd); continue; }

        // Receive filename
        char namebuf[2048];
        if (recv_all(client_fd, namebuf, name_len) <= 0) {
            close(client_fd); continue;
        }
        namebuf[name_len] = '\0';

        // Receive filesize
        int64_t filesize_net;
        if (recv_all(client_fd, &filesize_net, sizeof(filesize_net)) <= 0) {
            close(client_fd); continue;
        }
        int64_t filesize = be64toh(filesize_net);
        if (filesize < 0) { close(client_fd); continue; }

        printf("Cliente %s - Archivo: %s (%lld bytes)\n",
               client_ip, namebuf, (long long)filesize);

        // Receive and save file
        FILE *f = fopen(namebuf, "wb");
        if (!f) {
            perror("fopen");
            close(client_fd);
            continue;
        }

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
            unlink(namebuf);
            send(client_fd, "ERROR: Transfer incompleto\n", 27, 0);
            log_event(client_ip, namebuf, "TRANSFER ERROR");
            close(client_fd);
            continue;
        }

        // Process image with BOTH functions automatically
        char response[1024] = {0};
        char hist_output[512];
        int classify_result = 0, histogram_result = 0;

        printf("Procesando imagen %s...\n", namebuf);

        // 1. FIRST: Histogram Equalization (process original file before classification moves it)
        generate_histogram_filename(namebuf, hist_output, sizeof(hist_output));
        histogram_result = process_histogram_equalization(namebuf, hist_output);
        
        // 2. SECOND: Color Classification (this will move the original file to appropriate directory)
        classify_result = classify_image(namebuf, namebuf, DIR_ROJAS, DIR_VERDES, DIR_AZULES);

        // Generate response
        if (classify_result == 0 && histogram_result == 0) {
            snprintf(response, sizeof(response), 
                    "OK: Imagen clasificada y ecualizada exitosamente\nEcualizada: %s\n", 
                    hist_output);
            log_event(client_ip, namebuf, "BOTH OK");
        } else if (classify_result == 0) {
            snprintf(response, sizeof(response), 
                    "PARCIAL: Clasificación OK, Error en ecualización\n");
            log_event(client_ip, namebuf, "CLASSIFY OK, HISTOGRAM ERROR");
        } else if (histogram_result == 0) {
            snprintf(response, sizeof(response), 
                    "PARCIAL: Error clasificación, Ecualización OK: %s\n", hist_output);
            log_event(client_ip, namebuf, "CLASSIFY ERROR, HISTOGRAM OK");
        } else {
            snprintf(response, sizeof(response), 
                    "ERROR: Falló clasificación y ecualización\n");
            log_event(client_ip, namebuf, "BOTH ERROR");
        }

        send(client_fd, response, strlen(response), 0);
        close(client_fd);
        
        printf("Procesamiento completado para %s\n", namebuf);
    }

    close(server_fd);
    return 0;
}