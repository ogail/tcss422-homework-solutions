#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>
#include "utility.h"

#define MAX_LOG_LINE (512)
#define ENABLE_LOG true

int allocated_chunks = 0;
bool logging_enabled;
bool init = false;
pthread_mutex_t memory_mutex;
pthread_mutex_t logging_mutex;

void * safe_malloc(int size) {
	if (!init) {
		init = true;
		logging_enabled = ENABLE_LOG;
		pthread_mutex_init(&memory_mutex, NULL);
		pthread_mutex_init(&logging_mutex, NULL);
	}

	void * chunk = malloc(size);
	if (chunk == NULL) {
		printf("ERROR: memory allocation failed, exit\n");
		exit(1);
	}

	// increment number of created chunks
	pthread_mutex_lock(&memory_mutex);
	allocated_chunks++;
	pthread_mutex_unlock(&memory_mutex);


	// Set the memory chunk values to 0;
	memset(chunk, 0, size);

	return chunk;
}

void safe_free(void * chunk) {
	if (!init) {
		init = true;
		logging_enabled = ENABLE_LOG;
		pthread_mutex_init(&memory_mutex, NULL);
		pthread_mutex_init(&logging_mutex, NULL);
	}

	if (chunk != NULL) {
		free(chunk);
		chunk = NULL;

		// decrement number of created chunks
		pthread_mutex_lock(&memory_mutex);
		allocated_chunks--;
		pthread_mutex_unlock(&memory_mutex);
	}
}

FILE * safe_fopen(const char * file_name, const char * mode) {
	FILE * stream = fopen(file_name, mode);
	if (stream == NULL) {
		printf("ERROR: loading file at %s\n", file_name);
		exit(1);
	}

	return stream;
}

void safe_fread(void * buffer, int size, FILE * stream) {
	fread(buffer, size, 1, stream);
	if (buffer == NULL) {
		fclose(stream);
		exit(1);
	}
}

char * safe_strdup(const char * str) {
	if (!init) {
		init = true;
		logging_enabled = ENABLE_LOG;
		pthread_mutex_init(&memory_mutex, NULL);
		pthread_mutex_init(&logging_mutex, NULL);
	}

	char * dup = strdup(str);
	if (dup == NULL) {
		printf("ERROR: duplicating string %s\n", str);
		exit(1);
	}

	// increment number of created chunks
	pthread_mutex_lock(&memory_mutex);
	allocated_chunks++;
	pthread_mutex_unlock(&memory_mutex);

	return dup;
}

void safe_pthread_create(pthread_t * thread, const pthread_attr_t * attr, void * (*start_routine) (void *), void *arg) {
	int rv = pthread_create(thread, attr, start_routine, arg);
	if (rv != 0) {
		printf("ERROR: unable to create new thread.\n");
		exit(1);
	}
}

void write_log2(const char * msg, ...) {
	if (!init) {
		init = true;
		logging_enabled = ENABLE_LOG;
		pthread_mutex_init(&memory_mutex, NULL);
		pthread_mutex_init(&logging_mutex, NULL);
	}

	if (!logging_enabled) {
		return;
	}

	char buffer[MAX_LOG_LINE];
	va_list format_args;
	FILE * file_stream;

	// Format input message
	va_start(format_args, msg);
	vsprintf(buffer, msg, format_args);
	va_end(format_args);

	pthread_mutex_lock(&logging_mutex);
	file_stream = safe_fopen("log.txt", "a");
	fprintf(file_stream, buffer);
	fclose(file_stream);
	pthread_mutex_unlock(&logging_mutex);
}

void write_log1(const char * func, const char * msg, ...) {
	if (!init) {
		init = true;
		logging_enabled = ENABLE_LOG;
		pthread_mutex_init(&memory_mutex, NULL);
		pthread_mutex_init(&logging_mutex, NULL);
	}

	if (!logging_enabled) {
		return;
	}

	char buffer1[MAX_LOG_LINE];
	char buffer2[MAX_LOG_LINE];
	va_list format_args;
	FILE * file_stream;

	// Format input message
	va_start(format_args, msg);
	vsprintf(buffer1, msg, format_args);
	va_end(format_args);

	// Format time
	sprintf(buffer2, "[%s] %s\n", func, buffer1);

	pthread_mutex_lock(&logging_mutex);
	file_stream = safe_fopen("log.txt", "a");
	fprintf(file_stream, buffer2);
	fclose(file_stream);
	pthread_mutex_unlock(&logging_mutex);
}

unsigned int get_bits(unsigned int n, unsigned int start, unsigned count) {
	unsigned int result = 0;
	int k = start;
	int i, bit, bit_value;
	for(i = 0; i < count; i++, k++) {
		bit_value = pow(2, k);
		bit = ((n >> k)  & 1);
		result += bit * bit_value;
		// result += ((n >> k)  & 1) * pow(2, k)
	}

	return result;
}
