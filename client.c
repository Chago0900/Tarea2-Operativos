#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <stdint.h>
#include <endian.h>

#define DEFAULT_PORT 1717
#define BUFFER_SIZE 4096
#define MAX_FILENAME 1024

int64_t get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0) return (int64_t)st.st_size;
    return -1;
}

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

void print_usage(const char *prog_name) {
    printf("Uso: %s [ip_servidor] [puerto]\n", prog_name);
    printf("Si no se especifican, usa por defecto 127.0.0.1:%d\n", DEFAULT_PORT);
    printf("\nModo interactivo:\n");
    printf("- Ingresa nombres de archivos de imagen uno por uno\n");
    printf("- Escribe 'Exit' para terminar\n");
}

int send_image_to_server(const char *server_ip, int port, const char *filepath) {
    // Get base filename
    const char *base = strrchr(filepath, '/');
    if (base) base++; else base = filepath;
    uint32_t name_len = (uint32_t)strlen(base);

    int64_t filesize = get_file_size(filepath);
    if (filesize < 0) {
        printf("Error: No se pudo obtener tamaño del archivo %s\n", filepath);
        return -1;
    }

    FILE *f = fopen(filepath, "rb");
    if (!f) { 
        printf("Error: No se pudo abrir archivo %s\n", filepath);
        return -1; 
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { 
        perror("socket"); 
        fclose(f); 
        return -1; 
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &addr.sin_addr) <= 0) {
        printf("Error: Dirección IP inválida: %s\n", server_ip);
        close(sock); fclose(f); 
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("Error: No se pudo conectar al servidor %s:%d\n", server_ip, port);
        close(sock); fclose(f); 
        return -1;
    }

    // Send name_len
    uint32_t name_len_net = htonl(name_len);
    if (send_all(sock, &name_len_net, sizeof(name_len_net)) <= 0) {
        perror("send name_len");
        close(sock); fclose(f); 
        return -1;
    }

    // Send filename
    if (send_all(sock, base, name_len) <= 0) {
        perror("send name");
        close(sock); fclose(f); 
        return -1;
    }

    // Send filesize
    int64_t filesize_net = htobe64((int64_t)filesize);
    if (send_all(sock, &filesize_net, sizeof(filesize_net)) <= 0) {
        perror("send filesize");
        close(sock); fclose(f); 
        return -1;
    }

    // Send file content
    char buffer[BUFFER_SIZE];
    int64_t sent = 0;
    while (!feof(f)) {
        size_t n = fread(buffer, 1, sizeof(buffer), f);
        if (n > 0) {
            if (send_all(sock, buffer, n) <= 0) {
                perror("send file chunk");
                close(sock); fclose(f); 
                return -1;
            }
            sent += n;
        }
    }
    fclose(f);

    printf("→ Archivo enviado: %s (%lld bytes)\n", base, (long long)sent);

    // Receive response
    int r = recv(sock, buffer, sizeof(buffer)-1, 0);
    if (r > 0) {
        buffer[r] = '\0';
        printf("← Respuesta del servidor: %s", buffer);
    }

    close(sock);
    return 0;
}

int main(int argc, char *argv[]) {
    const char *server_ip = "127.0.0.1";
    int port = DEFAULT_PORT;
    
    // Parse command line arguments
    if (argc >= 2) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        server_ip = argv[1];
    }
    if (argc >= 3) {
        port = atoi(argv[2]);
        if (port <= 0 || port > 65535) {
            printf("Error: Puerto inválido %d\n", port);
            return 1;
        }
    }

    printf("Servidor: %s:%d\n", server_ip, port);
    printf("Instrucciones:\n");
    printf("- Ingrese el nombre del archivo de imagen\n");
    printf("- Escribe 'Exit' para terminar\n");
    printf("===============================================\n\n");

    char filename[MAX_FILENAME];
    int image_count = 0;
    
    while (1) {
        printf("Ingrese nombre de archivo de imagen (o 'Exit' para salir): ");
        fflush(stdout);
        
        if (!fgets(filename, sizeof(filename), stdin)) {
            break;
        }
        
        // Remove newline character
        size_t len = strlen(filename);
        if (len > 0 && filename[len-1] == '\n') {
            filename[len-1] = '\0';
        }
        
        // Check for exit condition
        if (strcasecmp(filename, "exit") == 0 || strcasecmp(filename, "Exit") == 0) {
            printf("Cerrando cliente...\n");
            break;
        }
        
        // Skip empty input
        if (strlen(filename) == 0) {
            continue;
        }
        
        printf("\nProcesando imagen #%d: %s ---\n", ++image_count, filename);
        
        if (send_image_to_server(server_ip, port, filename) == 0) {
            printf("✓ Imagen procesada exitosamente\n");
        } else {
            printf("✗ Error procesando imagen\n");
        }
        
        printf("\n");
    }
    
    printf("Total de imágenes procesadas: %d\n", image_count);
    
    return 0;
}