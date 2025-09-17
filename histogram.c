#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stb-master/stb_image.h"
#include "stb-master/stb_image_write.h"

void to_grayscale(unsigned char *original_data,unsigned char *new_data,  int width, int height){
 	int size = width * height;
	for (int i=0; i < size; i++){
		new_data[i] = 0.299 * original_data[3*i] + 0.587 * original_data[3*i +1] + 0.114 * original_data[3*i +2];
	}
	
}

void histogram_equalization(unsigned char* image,unsigned char* out_image, int width, int height){
	int pixel_intensities[256] = {0};
	float image_size = width*height;

	for (size_t i=0; i< (size_t) image_size; i++){
		pixel_intensities[image[i]] ++;
	}

	float cdf[256] = {0};
	unsigned char mapped_pixels[256] = {0};

	cdf[0] = pixel_intensities[0] / image_size;
	mapped_pixels[0] = cdf[0] * 255;

	for (int i=1; i < 256; i++){
		double pixel_intensity_probability = pixel_intensities[i] / image_size;
		cdf[i] = cdf[i-1] + pixel_intensity_probability;
		mapped_pixels[i] = cdf[i] * 255;
	}

	for (int i=0; i < image_size; i++){
		out_image[i] = mapped_pixels[image[i]];
	}
} 
	
int process_histogram_equalization(const char *input_filepath, const char *output_filepath){
	
	int width, height, channels;
	unsigned char *data = stbi_load(input_filepath, &width, &height, &channels, 0);
	
	if (!data) {
        fprintf(stderr, "Failed to load image for histogram: %s\n", input_filepath);
        return -1;
    }

	size_t size = (width * height);
	unsigned char *out = malloc(size);
	
	if (!out) {
		stbi_image_free(data);
		return -1;
	}

	if (channels > 1){
		unsigned char* gray = malloc((size_t) width*height);
		if (!gray) {
			free(out);
			stbi_image_free(data);
			return -1;
		}
		to_grayscale(data, gray, width, height);
		histogram_equalization(gray, out, width, height);	
		free(gray);
	}else{
		histogram_equalization(data, out, width, height);
	}

	int result = 0;

	// Guarda el archivo con extension correcta
    if (strstr(input_filepath, ".png") || strstr(input_filepath, ".PNG")) {
        if (!stbi_write_png(output_filepath, width, height, 1, out, width)) {
            result = -1;
        }
    } else if (strstr(input_filepath, ".jpg") || strstr(input_filepath, ".jpeg") || 
               strstr(input_filepath, ".JPG") || strstr(input_filepath, ".JPEG")) {
        if (!stbi_write_jpg(output_filepath, width, height, 1, out, 90)) {
            result = -1;
        }
    } else {
        if (!stbi_write_png(output_filepath, width, height, 1, out, width)) {
            result = -1;
        }
    }
	stbi_image_free(data);
	free(out);
	return result;
}
