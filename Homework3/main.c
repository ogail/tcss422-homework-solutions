#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "utility.h"
#include "analyzer.h"

void show_usage(const char* exe);

int main(int argc, char* argv[]) {
	int thread_limit;

	// Process command-line arguments.
	if (argc == 3) {
		char * end;
		thread_limit = strtol(argv[1], &end, 10);
		if (*end != 0) {
			show_usage(argv[0]);
			return EXIT_FAILURE;
		}
		if (thread_limit < 1) {
			show_usage(argv[0]);
			return EXIT_FAILURE;
		}
	} else {
		show_usage(argv[0]);
		return (argc == 2 && strcmp(argv[1], "--help") == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	init_utility();

	// Ready to start retrieving and analyzing images.
	int images;
	image_info * results = analyze_images_in_directory(thread_limit, argv[2], &images);
	printf("%d image(s) analyzed.\n", images);
	int i;
	for (i = 0; i < images; i++) {
		printf("%s  (%d,%d)-(%d,%d)\n", results[i].path, results[i].top_left_x, results[i].top_left_y, results[i].bottom_right_x, results[i].bottom_right_y);
		safe_free(results[i].path);
	}
	safe_free(results);

	// Verify that all allocated memory has been freed ahead
	assert(allocated_chunks == 0);
	destroy_utility();

	return EXIT_SUCCESS;
}

void show_usage(const char* exe) {
	printf("Usage: %s <thread-limit> <directory>\n", exe);
}
