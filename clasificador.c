#include "stb-master/stb_image.h"
#include "stb-master/stb_image_write.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define MAX_PATH 4096

// Determina el color predominante de una imagen (r/g/b)
static char predominant_color(const char *filepath) {
    int width, height, channels;
    unsigned char *img = stbi_load(filepath, &width, &height, &channels, 3); 
    // fuerza 3 canales (RGB)

    if (!img) {
        fprintf(stderr, "Error cargando imagen %s\n", filepath);
        return 'g'; // por defecto verde
    }

    unsigned long long r_sum = 0, g_sum = 0, b_sum = 0;
    long total_pixels = width * height;

    for (long i = 0; i < total_pixels; i++) {
        r_sum += img[i * 3 + 0];
        g_sum += img[i * 3 + 1];
        b_sum += img[i * 3 + 2];
    }

    stbi_image_free(img);

    if (r_sum >= g_sum && r_sum >= b_sum) return 'r';
    else if (g_sum >= r_sum && g_sum >= b_sum) return 'g';
    else return 'b';
}

// Clasifica la imagen moviéndola a rojas/, verdes/ o azules/
int classify_image(const char *filepath,
                          const char *filename,
                          const char *dir_rojas,
                          const char *dir_verdes,
                          const char *dir_azules) {
    char dest[MAX_PATH];
    char color = predominant_color(filepath);

    if (color == 'r')
        snprintf(dest, sizeof(dest), "%s/%s", dir_rojas, filename);
    else if (color == 'g')
        snprintf(dest, sizeof(dest), "%s/%s", dir_verdes, filename);
    else
        snprintf(dest, sizeof(dest), "%s/%s", dir_azules, filename);

    // Mover archivo al destino
    if (rename(filepath, dest) != 0) {
        fprintf(stderr, "Error moviendo %s -> %s: %s\n",
                filepath, dest, strerror(errno));
        return -1;
    }

    printf("Imagen %s clasificada en %s\n", filename, dest);
    return 0;
}
