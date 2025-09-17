#ifndef HISTOGRAM_H
#define HISTOGRAM_H

void to_grayscale(unsigned char *original_data, unsigned char *new_data, int width, int height);
void histogram_equalization(unsigned char* image, unsigned char* out_image, int width, int height);
int process_histogram_equalization(const char *input_filepath, const char *output_filepath);

#endif