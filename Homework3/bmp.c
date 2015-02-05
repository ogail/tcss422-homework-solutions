#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "bmp.h"

#define BYTES_PER_PIXEL (3)

void load_bmp(const char * file, bmp * image) {
	FILE * file_stream = NULL;
	byte * raw_pixels = NULL;
	uint i, pixels_count, row_size, padding, padded_row_size;
	int j, k, index;

	image->path = safe_strdup(file);
	file_stream = safe_fopen(file, "rb");
	safe_fread(&image->header, sizeof(bmp_header), file_stream);

	if (image->header.type != 0x4D42)
	{
		fclose(file_stream);
		return;
	}

	assert(BYTES_PER_PIXEL == image->header.bit_count / 8);

	fseek(file_stream, image->header.pixels_address, SEEK_SET);
	pixels_count = image->header.width * image->header.height;
	row_size = image->header.bit_count * image->header.width;
	padded_row_size = ((row_size + 31) / 32) * 4;
	row_size /= BITS_PER_BYTE;
	padding = padded_row_size - row_size;
	raw_pixels = safe_malloc(image->header.image_size);
	image->pixels = safe_malloc(pixels_count * sizeof(int));
	safe_fread(raw_pixels, image->header.image_size, file_stream);
	for (i = 0, j = pixels_count - image->header.width; j >= 0; j -= image->header.width) {
		for (k = 0; k < image->header.width; k++, i += 3) {
			index = j + k;
			if (index != 0 && index % image->header.width == 0) {
				// Skip padding bits
				i += padding;
			}

			assert(i + 2 < image->header.image_size - padding);

			image->pixels[index] = raw_pixels[i];
			image->pixels[index] = (image->pixels[index] << BITS_PER_BYTE) | raw_pixels[i + 1];
			image->pixels[index] = (image->pixels[index] << BITS_PER_BYTE) | raw_pixels[i + 2];
		}
	}

	assert(i == image->header.image_size - padding);
	safe_free(raw_pixels);
	//dump_bmp(image);
}

void dump_bmp(const bmp * image) {
	int i;
	write_log2("\nDumbing image width=%d and height=%d from path %s:\n", image->header.width, image->header.height, image->path);

	for (i = 0; i < image->header.width * image->header.height; i++) {
		if (i != 0 && i % image->header.width == 0) {
			write_log2("%d\n", image->pixels[i]);
		} else {
			write_log2("%d,", image->pixels[i]);
		}
	}
}

int get_pixel(int row, int column, const bmp * image) {
	return image->pixels[(row * image->header.width) + column];
}
