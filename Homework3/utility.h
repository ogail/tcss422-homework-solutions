#ifndef _UTILITY_H_
#define _UTILITY_H_

#define true (1)
#define false (0)
#define BITS_PER_BYTE (8)

typedef int bool;
typedef unsigned char byte;

extern int allocated_chunks;

/* MEMORY MANAGEMENT APIs */

void * safe_malloc(int size);

void safe_free(void * chunk);

/* DISK MANAGEMENT APIs */

FILE * safe_fopen(const char * file_name, const char * mode);

void safe_fread(void * buffer, int size, FILE * stream);

char * safe_strdup(const char * str);

/* THREAD MANAGEMENT APIs */

void safe_pthread_create(pthread_t * thread, const pthread_attr_t * attr, void * (*start_routine) (void *), void *arg);

/* LOGGING MANAGEMENT APIs */

void write_log1(const char * func, const char * msg, ...);

void write_log2(const char * msg, ...);

#define LOG_MSG(format, ...) write_log1(__FUNCTION__, format, __VA_ARGS__)

#endif // _UTILITY_H_
