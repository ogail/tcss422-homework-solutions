#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "lexicon.h"

typedef int bool;
#define true 1
#define false 0
#define LINE_MAX (80)
#define DECK_SIZE (52)
#define MAX_WORD_SIZE (16)

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
    int length = strlen(str);
    amount = (amount % length);
    char* nStr = malloc(length + 1);
    int start = amount >= 0 ? length - amount : abs(amount);
    int i;

    for (i = 0; i < length; ++i)
    {
        nStr[i] = str[(start + i) % length];
    }

    nStr[length] = 0;

    return nStr;
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
    char line[LINE_MAX];
    FILE *fr = fopen(path, "rt");
    char* token;
    int counter = 0;
    int length;

    while (fgets(line, LINE_MAX, fr) != NULL)
    {
        length = strlen(line);
        line[length - 1] = 0; // Get rid of the trailing \n
        token = strtok(line, ",");
        counter = 0;
        while (token)
        {
            switch (counter++)
            {
                case 0: printf("\"%s\" by", token); break;
                case 1: printf(" %s", token); break;
                case 2: printf(" (%s)", token); break;
                default: assert(!"not supported phrase!"); break; // shall not reach this
            }
            token = strtok(NULL, ",");
        }
        printf("\n");
    }
    
    return EXIT_SUCCESS;
}

void test_readAndDisplayBookInformation() {
	readAndDisplayBookInformation("books.txt");
}

struct Card {
	char rank;
	char suit;
};

void initializeAndShuffleDeck(struct Card deck[52]) {
    char ranks[] = { '2', '3', '4', '5', '6', '7', '8', '9', '0', 'J', 'Q', 'K', 'A' };
    char suites[] = {'c', 'd', 'h', 's'};
    int i = 0;
    int j, k;
    int src, dest;
    struct Card card;

    // Initialize
    for (j = 0; j < sizeof(ranks); ++j)
    {
        card.rank = ranks[j];
        for (k = 0; k < sizeof(suites); ++k)
        {
            card.suit = suites[k];
            deck[i++] = card;
        }
    }

    // Shuffle
    srand(time(NULL));
    for (i = 0; i < DECK_SIZE; ++i)
    {
        src = rand() % DECK_SIZE;
        dest = rand() % DECK_SIZE;
        card = deck[dest];
        deck[dest] = deck[src];
        deck[src] = card;
    }
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

void findWord(int row, int column, int wordSize, char * oldWord, const char board[4][4], struct ListNode ** words)
{
    // base case that we are out of boundaries or reached 17 chars without match
    if (row > 3 || column > 3 || row < 0 || column < 0 || wordSize > MAX_WORD_SIZE)
        return;

    char * word = malloc(wordSize + 1);
    if (oldWord != NULL)
    {
        strcpy(word, oldWord);
    }

    word[wordSize - 1] = board[row][column];
    word[wordSize++] = '\0'; // NULL terminated char
    
    if (isWord(word))
    {
        struct ListNode * curr = malloc(sizeof(struct ListNode));
        curr->word = strdup(word);
        curr->next = *words;
        *words = curr;
    }

    if (isPrefix(word))
    {
        // up left
        findWord(row - 1, column - 1, wordSize, word, board, words);
        
        // up
        findWord(row - 1, column, wordSize, word, board, words);
        
        // up right
        findWord(row - 1, column + 1, wordSize, word, board, words);
        
        // left
        findWord(row, column - 1, wordSize, word, board, words);
        
        // right
        findWord(row, column + 1, wordSize, word, board, words);
        
        // down left
        findWord(row + 1, column - 1, wordSize, word, board, words);
        
        // down
        findWord(row + 1, column, wordSize, word, board, words);
        
        // down right
        findWord(row + 1, column + 1, wordSize, word, board, words);
    }

    free(word);
}

struct ListNode * findWords(const char board[4][4]) {
    struct ListNode * words = NULL;
    int wordSize, i, j;
    char * word = NULL;

    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            wordSize = 1;
            findWord(i, j, wordSize, word, board, &words);
        }
    }

    return words;
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
	test_rotate();
    test_readAndDisplayBookInformation();
	test_initializeAndShuffleDeck();
	test_findWords();
	return EXIT_SUCCESS;
}
