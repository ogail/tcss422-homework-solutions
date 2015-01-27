#ifdef __APPLE__
#include <sys/time.h>
#define STOPWATCH_TYPE struct timeval
#define STOPWATCH_CLICK(x) gettimeofday(&x, NULL)
#else
#define _POSIX_C_SOURCE 199309
#include <time.h>
#define STOPWATCH_TYPE struct timespec
#define STOPWATCH_CLICK(x) clock_gettime(CLOCK_REALTIME, &x)
#endif
#include <pthread.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define DEFAULT_THREADS (4)

typedef struct {
	int id; // thread ID (for debugging purpose)
	int start_row;
	int end_row;
	int r;
	int s;
	int t;
	const int * A;
	const int * B;
	int * C;
} matrix_mul_arg;

void* matrix_mul_wrapper(void* t_arg);
long ms_diff(STOPWATCH_TYPE start, STOPWATCH_TYPE stop);
void matrix_mul(const int A[], const int B[], int C[], int r, int s, int t, int start_row, int end_row);

int * multithreaded_matrix_product(const int A[], const int B[], int r, int s, int t) {
	int num_threads = fmin(r, DEFAULT_THREADS); // this is to handle case when number of rows is less than DEFAULT_THREADS (4)
	int chunk = r / num_threads;
	int * C = (int *)malloc(r * t * sizeof(int));
	matrix_mul_arg mm_args[num_threads];
	pthread_t threads[num_threads];
	int i;

	memset (C, 0, r * t * sizeof(int));

	printf("num_threads=%d\n", num_threads);
	for (i = 0; i < num_threads; i++) {
		mm_args[i].id = i;
		mm_args[i].r = r;
		mm_args[i].s = s;
		mm_args[i].t = t;
		mm_args[i].start_row = i * chunk;
		mm_args[i].end_row = mm_args[i].start_row + chunk - 1;
		mm_args[i].A = A;
		mm_args[i].B = B;
		mm_args[i].C = C;
		if (pthread_create(&threads[i], NULL, matrix_mul_wrapper, &mm_args[i]) != 0) {
			printf("ERROR: Unable to create new thread.\n");
			exit(EXIT_FAILURE);
		}
	}

	mm_args[i - 1].end_row += r % num_threads; // this is to handle left over matrix rows in case that r is not divisible by num_threads.
	assert(mm_args[i - 1].end_row == r - 1);

	for (i = 0; i < num_threads; i++) {
		pthread_join(threads[i], NULL);
	}

	return C;
}

// Creates an r-by-s matrix with random elements in the range [-5, 5].
int * create_random_matrix(int r, int s) {
	int size = r * s;
	int * matrix = (int *)malloc(size * sizeof(int));
	int i;
	for (i = 0; i < size; i++)
		matrix[i] = rand() % 11 - 5;
	return matrix;
}

void print_matrix(int * M, int r, int t)
{
	int i;
	for (i = 1; i <= r * t; i++)
	{
		printf("%d ", M[i - 1]);
		if (i % t == 0) printf("\n");
	}

	printf("\n");
}

int main(int argc, char* argv[]) {
	srand(time(NULL));

	const int r = 5;//1000;
	const int s = 2;//2000;
	const int t = 4;//1000;
	int * A = create_random_matrix(r, s);
	int * B = create_random_matrix(s, t);
	STOPWATCH_TYPE start;
	STOPWATCH_CLICK(start);
	int * C = multithreaded_matrix_product(A, B, r, s, t);
	STOPWATCH_TYPE stop;
	STOPWATCH_CLICK(stop);
	//print_matrix(A, r, s);
	//print_matrix(B, s, t);
	//print_matrix(C, r, t);
	printf("%ld ms elapsed.\n", ms_diff(start, stop));
	free(A);
	free(B);
	free(C);

	return EXIT_SUCCESS;
}

void* matrix_mul_wrapper(void* t_arg) {
	const matrix_mul_arg* arg = t_arg;
	printf("thread[%d]: %d-%d.\n", arg->id, arg->start_row, arg->end_row);
	matrix_mul(arg->A, arg->B, arg->C, arg->r, arg->s, arg->t, arg->start_row, arg->end_row);

	return NULL;
}

void matrix_mul(const int A[], const int B[], int C[], int r, int s, int t, int start_row, int end_row) {
	int a_start_index = start_row * s;
	int b_start_index = 0;
	int c_current_index = start_row * t;
	int c_last_index = (end_row + 1) * t;
	int a_current_index;
	int b_current_index;
	int a_value, b_value, c_value;

	//printf("c_last_index = %d\n", c_last_index);
	while (c_current_index < c_last_index)
	{
		b_current_index = 0;
		while (b_current_index - b_start_index < t)
		{
			a_current_index = a_start_index;
			while (a_current_index - a_start_index < s)
			{
				a_value = A[a_current_index];
				b_value = B[b_current_index + ((a_current_index - a_start_index)  * t)];
				c_value = a_value * b_value;
				C[c_current_index] += c_value;
				//printf("A[%d](%d) * B[%d](%d) ", a_current_index, a_value, b_current_index + ((a_current_index - a_start_index)  * t), b_value);
				a_current_index++;
			};
			//printf("\t\t");
			c_current_index++;
			b_current_index++;
		};
		a_start_index += s;
		//printf("\n");
	}
}

#ifdef __APPLE__
long ms_diff(struct timeval start, struct timeval stop) {
	return 1000L * (stop.tv_sec - start.tv_sec) + (stop.tv_usec - start.tv_usec) / 1000;
}
#else
long ms_diff(struct timespec start, struct timespec stop) {
	return 1000L * (stop.tv_sec - start.tv_sec) + (stop.tv_nsec - start.tv_nsec) / 1000000;
}
#endif
