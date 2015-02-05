#ifndef _ANALYZER_H_
#define _ANALYZER_H_

#include <pthread.h>
#include "utility.h"

typedef struct bmp_t bmp;

struct ImageInfo {
	// Path to the image file.
	// The memory for this string should be allocated dynamically.
	char * path;
	// Coordinates of the top-left corner of the largest rectangle of
	// uniform color.
	int top_left_x, top_left_y;
	// Coordinates of the bottom-right corner of the largest rectangle of
	// uniform color.
	int bottom_right_x, bottom_right_y;
};

typedef struct ImageInfo image_info;

// The last image index available for processing
int available_images_count;

// Total number of images to analyze
int total_images_count;

// Total number of images to analyzed till current moment
int analyzed_images_count;

// Data structure to hold the analyzed images.
image_info * images_info;

// Data structure to hold the bitmaps
bmp * images;

// Mutex to lock the critical section in analyze_image and load_images
pthread_mutex_t images_mutex;

// Mutex to lock the critical section in analyze_image and load_images
pthread_mutex_t images_info_mutex;

// Returns a dynamically allocated array of ImageInfo structs, one per image.
// The number of valid elements in the array should be written to the address
// in parameter images_analyzed.
// Processes images in the directory and detect the largesr rectangle of same color in every image.
image_info * analyze_images_in_directory(int thread_limit, const char * directory, int * images_analyzed);

// Returns a dynamically allocated array of ImageInfo structs, one per image.
// The number of valid elements in the array should be written to the address
// in global variable images_index.
// Iterates over the directory and loads the available *.bmp images.
void * load_images(void * arg);

// Returns an ImageInfo struct for one image filled with coordinates of largest
// rectangle of same color.
void * analyze_image(void * arg);

// Iterates over a directory and lists all files there and use function
// pointer passed to apply some action on the files.
void read_dir(const char * directory, void (*file_action)(const char *));

// Utility function used with read_dir to count number of bmp files
// in a directory.
void count_images(const char * file_name);

// Utility function used with read_dir to load bmp image.
void load_image(const char * file_name);

// Finds maximum rectangle of contiguous color in an image
image_info get_max_rectangle(const bmp * image);

// Calculates area of a rectangle
int calc_area(int columnFrom, int columnTo, int rowFrom, int rowTo);

// Calculates the area of the contiguous rectangle in the specific coordinates
int get_area(const bmp * image, int columnFrom, int columnTo, int rowFrom, int rowTo);

#endif // _ANALYZER_H_
