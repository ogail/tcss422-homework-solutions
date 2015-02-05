#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include "analyzer.h"
#include "bmp.h"

image_info * analyze_images_in_directory(int thread_limit, const char * directory, int * images_analyzed) {
	uint i;
	pthread_t * analyzers = safe_malloc(thread_limit * sizeof(pthread_t));
	pthread_t loader;
	available_images_count = 0;
	total_images_count = 0;
	analyzed_images_count = 0;
	images = NULL;
	pthread_mutex_init(&images_mutex, NULL);
	pthread_mutex_init(&images_info_mutex, NULL);

	// Create images loader thread
	safe_pthread_create(&loader, NULL, load_images, (void *)directory);
	LOG_MSG("%s Thread created", "Loader");

	// Create images analyzer threads
	for (i = 0; i < thread_limit; i++) {
		safe_pthread_create(&analyzers[i], NULL, analyze_image, (void *) (intptr_t)(i + 1));
		LOG_MSG("Thread %d created", i + 1);
	}

	// Load and analyze
	pthread_join(loader, NULL);

	LOG_MSG("Loading images is done, total count is %d", total_images_count);

	*images_analyzed = total_images_count;
	while (total_images_count != analyzed_images_count);

	LOG_MSG("Analyzing images is done, total count is %d", analyzed_images_count);

	assert(total_images_count == analyzed_images_count);
	assert(analyzed_images_count == *images_analyzed);

	// cleanup
	for (i = 0; i < thread_limit; i++) {
		pthread_cancel(analyzers[i]);
	}

	pthread_mutex_destroy(&images_info_mutex);
	pthread_mutex_destroy(&images_mutex);
	safe_free(analyzers);
	safe_free(images);

	return images_info;
}

void * analyze_image(void * arg) {
	bmp image;
	image_info analyzed_image;
	uint id = (intptr_t)arg;
	int src_index, dest_index;
	while (true) {
		src_index = INT_MIN;
		dest_index = INT_MIN;
		pthread_mutex_lock(&images_mutex);
		if (available_images_count > 0) {
			available_images_count--;
			src_index = available_images_count;
			image = images[src_index];
			LOG_MSG("Thread %d: will process image %s", id, image.path);
			LOG_MSG("Thread %d: available images to process %d", id, available_images_count);
		}
		pthread_mutex_unlock(&images_mutex);

		if (src_index != INT_MIN) {
			assert(image.pixels != NULL);
			assert(image.path != NULL);

			LOG_MSG("Thread %d: started processing image %s", id, image.path);

			analyzed_image = get_max_rectangle(&image);

			LOG_MSG("Thread %d: finished processing image %s", id, image.path);

			pthread_mutex_lock(&images_info_mutex);
			dest_index = analyzed_images_count;
			analyzed_image.path = safe_strdup(image.path);
			images_info[dest_index] = analyzed_image;
			safe_free(image.path);
			safe_free(image.pixels);
			LOG_MSG("Thread %d: saved image %s in index %d", id, analyzed_image.path, dest_index);
			LOG_MSG("Thread %d: analyzed images %d", id, analyzed_images_count + 1);
			analyzed_images_count++;
			pthread_mutex_unlock(&images_info_mutex);
		}
	}

	return NULL;
}

void * load_images(void * arg) {
	const char * directory = (const char *)arg;
	read_dir(directory, count_images);

	if (total_images_count > 0) {
		images_info = safe_malloc((total_images_count) * sizeof(image_info));
		images = safe_malloc((total_images_count) * sizeof(bmp));
		read_dir(directory, load_image);
	}

	return NULL;
}

void read_dir(const char * directory, void (*file_action)(const char *)) {
	uint length;
	struct dirent *dirent;
	DIR *dir;

	dir = opendir(directory);
	if (dir != NULL) {
	    while ((dirent = readdir(dir)) != NULL) {
	    	length = strlen (dirent->d_name);
			if (length >= 4) {
				if (strcmp (".bmp", &(dirent->d_name[length - 4])) == 0) {
					char path[PATH_MAX + 1];
					realpath(dirent->d_name, path);
					file_action(path);
				}
			}
	    }
	    closedir(dir);
	}
}

void count_images(const char * file_name) {
	total_images_count++;
}

void load_image(const char * file_name) {
	bmp image;
	load_bmp(file_name, &image);
	LOG_MSG("Loading image from %s", file_name);
	pthread_mutex_lock(&images_mutex);
	images[available_images_count++] = image;
	LOG_MSG("image from %s loaded in index %d", file_name, available_images_count - 1);
	pthread_mutex_unlock(&images_mutex);
}

image_info get_max_rectangle(const bmp * image) {
	int row = image->header.height;
	int rowFrom, rowTo, columnFrom, columnTo;
	int max = 0;
	int area;
	int column = image->header.width;
	image_info analyzed_image;

	if(row == 0) {
		// if row equals to 0 then there's no pixels at all
		// means that column is equal to 0 as well.
		assert(column == 0);
		analyzed_image.top_left_x = 0;
		analyzed_image.top_left_y = 0;
		analyzed_image.bottom_right_x = 0;
		analyzed_image.bottom_right_y = 0;
	}

	for (rowFrom = 0; rowFrom < row; rowFrom++) {
		for (rowTo = rowFrom; rowTo < row; rowTo++) {
			for (columnFrom = 0; columnFrom < column; columnFrom++) {
				for (columnTo = columnFrom; columnTo < column; columnTo++) {
					area = get_area(image, columnFrom, columnTo, rowFrom, rowTo);

					if (area > max) {
						max = area;
						analyzed_image.top_left_x = columnFrom;
						analyzed_image.top_left_y = rowFrom;
						analyzed_image.bottom_right_x = columnTo;
						analyzed_image.bottom_right_y = rowTo;
					}
					else if(area == max && rowFrom == analyzed_image.top_left_y) {
						analyzed_image.top_left_x = columnFrom;
						analyzed_image.bottom_right_x = columnTo;
					}
				}
			}
		}
	}

	return analyzed_image;
}

int calc_area(int columnFrom, int columnTo, int rowFrom, int rowTo) {
	return (columnTo - columnFrom + 1) * (rowTo - rowFrom + 1);
}

int get_area(const bmp * image, int columnFrom, int columnTo, int rowFrom, int rowTo) {
	int i, j;
	int color = get_pixel(rowFrom, columnFrom, image);

	for (i = rowFrom; i <= rowTo; i++) {
		for (j = columnFrom; j <= columnTo; j++) {
			if (get_pixel(i, j, image) != color) return 0;
		}
	}

	return calc_area(columnFrom, columnTo, rowFrom, rowTo);
}
