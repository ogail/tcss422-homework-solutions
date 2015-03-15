#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>

#define PROC_MEMORY_SPACE (11)
#define UNDEFINED_PID (-999999)

int total = 0;
int max = 0;
int min = INT_MAX;
int min_process = UNDEFINED_PID;
int max_process = UNDEFINED_PID;
int proc_counter = 1;

void read_file(const char * path, int pid) {
	ssize_t read;
	size_t len;
	char * line = NULL;
	char * end = NULL;
	FILE * f = fopen(path, "r");
	int bytes;
	int space_counter = PROC_MEMORY_SPACE;
	int memory_index = 0;

	if (f) {
		// Read the file contents
		read = getline(&line, &len, f);

		// Assert read is successful
		assert(read != -1);

		// Find out the start index of the process total memory field
		while (space_counter > 0) {
			if (line[memory_index] == ' ') {
				--space_counter;
			}

			memory_index++;
		}

		// Convert the string to int
		bytes = strtol(line + memory_index, NULL, 0);

		// Increment number of total processes
		proc_counter++;

		// Add the memory usage to the total usage
		total += bytes;

		// Find out if the current usage is more than current max
		max = bytes > max ? bytes : max;
		max_process = bytes == max ? pid : max_process;

		// Find out if the current usage is less than current min
		min = bytes < min ? bytes : min;
		min_process = bytes == min ? pid : min_process;

		fclose (f);

		if (line) {
			free(line);
		}
	}
}

int main(int argc, char* argv[]) {
	struct dirent * dirent;
	DIR * dir;
	int r;
	int pid;
	char path[PATH_MAX];

	if (!(dir = opendir ("/proc"))) {
		fprintf (stderr, "%s:  couldn't open /proc, errno %d\n", "proc_info", EXIT_FAILURE);
		perror (NULL);
		exit (EXIT_FAILURE);
	}

	do {
		dirent = readdir (dir);
		// The first check works for detecting process ids with positive number
		// The second check works for negative process ids.
		if ((isdigit(*dirent->d_name)) ||
			(dirent->d_name[0] == '-' && isdigit(*(dirent->d_name + 1))) ) {
			memset(path, 0, sizeof(path));
			sprintf(path, "/proc/%s/psinfo", dirent->d_name);
			read_file(path, atoi(dirent->d_name));
		}
	} while (dirent);

	closedir (dir);

	printf("Total memory used by all processes: %d bytes\n", total);
	printf("The average memory used per process: %d bytes\n", (total / proc_counter));
	printf("The most memory used by any process (PID:%d): %d bytes\n", max_process, max);
	printf("The least memory used by any process (PID:%d): %d bytes\n", min_process, min);

	return EXIT_SUCCESS;
}
