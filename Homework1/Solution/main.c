#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "lexicon.h"

void multiply(int num1, int denom1, int num2, int denom2, int * rNum, int * rDenom) {
    int numProduct, demProduct, negative, smaller;
    assert(denom1 != 0 && denom2 != 0); // denominators can not be equal to zero
    numProduct = num1 * num2;
    demProduct = denom1 * denom2;
    negative = numProduct < 0 || demProduct < 0;
    smaller = numProduct < demProduct ? numProduct : demProduct;

    while (smaller != 0)
    {
        if (numProduct % smaller == 0 && demProduct % smaller == 0)
        {
            numProduct /= smaller;
            demProduct /= smaller;
            break;
        }
        --smaller;
    }

    if (negative)
    {
        numProduct *= -1;
    }

    *rNum = numProduct;
    *rDenom = demProduct;
}

void test_multiply() {
	int num, denom;
	multiply(2, 3, 7, 5, &num, &denom);
	printf("2/3 * 7/5 = %d/%d\n", num, denom);
	multiply(14, 9, 3, 2, &num, &denom);
	printf("14/9 * 3/2 = %d/%d\n", num, denom);
	multiply(8, 6, 12, 20, &num, &denom);
	printf("8/6 * 12/20 = %d/%d\n", num, denom);
}

char * rotate(const char * str, int amount) {
	return NULL;
}

void test_rotate() {
	char message[] = "Hello";
 	int i;
	for (i = 0; i <= 10; i++) {
		char * tmp = rotate(message, i);
		printf("%s\n", tmp);
		free(tmp);
	}
	for (i = -1; i >= -10; i--) {
		char * tmp = rotate(message, i);
		printf("%s\n", tmp);
		free(tmp);
	}
}

int readAndDisplayBookInformation(const char * path) {
	return 0;
}

void test_readAndDisplayBookInformation() {
	readAndDisplayBookInformation("books.txt");
}

struct Card {
	char rank;
	char suit;
};

void initializeAndShuffleDeck(struct Card deck[52]) {
}

void test_initializeAndShuffleDeck() {
	struct Card deck[52];
	initializeAndShuffleDeck(deck);
	int i;
	for (i = 0; i < 52; i++) {
		printf("%c%c ", deck[i].rank, deck[i].suit);
		if (i % 13 == 12)
			printf("\n");
	}
}

struct ListNode {
	char * word;
	struct ListNode * next;
};

struct ListNode * findWords(const char board[4][4]) {
	return NULL;
}

void test_findWords() {
	loadLexicon("words.txt");
	const char board[4][4] = {{'d', 'h', 'h', 'i'}, {'j', 'e', 'p', 's'}, {'i', 't', 'z', 't'}, {'a', 'l', 'm', 't'}};
	struct ListNode * words = findWords(board);
	struct ListNode * current = words;
	while (current != NULL) {
		printf("%s\n", current->word);
		current = current->next;
	}
	current = words;
	while (current != NULL) {
		words = current->next;
		free(current->word);
		free(current);
		current = words;
	}
	destroyLexicon();
}

int main(int argc, char* argv[]) {
 	test_multiply();
//	test_rotate();
//	test_readAndDisplayBookInformation();
//	test_initializeAndShuffleDeck();
//	test_findWords();
	return EXIT_SUCCESS;
}
