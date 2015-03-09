#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROGRAM_INPUT (5120)

int main(int argc, char* argv[]) {
	char * input = malloc(MAX_PROGRAM_INPUT);
	char * tmp_input = NULL;
	int i = 0;
	int j = 0;
	int ch;

	if (input == NULL) {
		return EXIT_FAILURE;
	}

	memset(input, 0, MAX_PROGRAM_INPUT);

	while (1) {
		ch = fgetc(stdin);
		if (feof(stdin))
		{
			break;
		}

		if (i >= MAX_PROGRAM_INPUT || i + 1 >= MAX_PROGRAM_INPUT) {
			tmp_input = malloc(strlen(input) * 2);
			strcpy(tmp_input, input);
			free(input);
			input = tmp_input;
		}

		input[i++] = ch;
	}

	i--;

	while (j < i) {
		char tmp = input[j];
		input[j++] = input[i];
		input[i--] = tmp;
	}

	printf("%s", input);

	free(input);

	return EXIT_SUCCESS;
}
