#ifndef _BMP_H_
#define _BMP_H_

#include "utility.h"

#pragma pack(push, 1)
typedef struct {
	ushort type;
	uint size;
	uint reserved;
	uint pixels_address;
	uint header_size; // must be 40
	uint width;
	uint height;
	ushort planes; // must be 1
	ushort bit_count; // in this assignment must be 1
	uint compression; // in this assignment must be 0
	uint image_size; //
	uint x_pixels_per_meter;
	uint y_pixels_per_meter;
	uint colors;
	uint important_colors;
} bmp_header;
#pragma pack(pop)

typedef int pixel;

// Structure to hold bitmap image information.
struct bmp_t {
	// The bitmap file path
	char * path;
	// The bitmap header info
	bmp_header header;
	// The bitmap pixels, where every pixel is represented by
	// an integer 1 byte for every color in this order with a padded
	// zero at the most significant byte: 0BGR
	pixel * pixels;
};

typedef struct bmp_t bmp;

void load_bmp(const char * file, bmp * image);

void dump_bmp(const bmp * image);

int get_pixel(int row, int column, const bmp * image);

#endif // _BMP_H_
