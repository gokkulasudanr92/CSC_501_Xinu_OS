#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <time.h>

#define MAX_NUM_OF_THREADS 500

int res[MAX_NUM_OF_THREADS];

struct thread_data {
	char *text;
	char *pattern;
	int count;
};

struct func_data {
	int thread_num;
	int start;
	int end;
	int blockSize;
	int count;
	char *text;
	char *pattern;
	int *prefix_table;
};

void *string_search_function(void *segment_data) {
	struct func_data *in;
	in = (struct func_data *) segment_data;
	int check = 0;
	
	//Naive Approach
	int i, j, m = strlen(in->pattern);
	for (i = in->start; i < in->end; i++) {
		check = 1;
		for (j = 0; j < m; j++) {
			if (in->text[i + j] != in->pattern[j]) {
				check = 0;
				break;
			}
		}
		
		if (j == m) {
			res[(in->thread_num - 1)] += 1;
		}
	}
	
	if (check && (in->end + m - j < in->count)) {
		for (;i < in->end + m - j; i ++) {
			for (; j < m; j++) {
				if (in->text[i + j] != in->pattern[j]) {
					break;
				}
			}
			
			if (j == m) {
				res[(in->thread_num - 1)] += 1;
			}
		}
	}
	
	return NULL;
}

void *manager_thread(void *info) {
	int result;
	
	struct thread_data *in;
	in = (struct thread_data *) info;
	
	char *pattern;
	pattern = in->pattern;
	//Pattern Count is calculated
	int pattern_count = (int) strlen(pattern);
	
	// Compute the prefix table for the given pattern
	int *prefix_table;
	prefix_table = (int *) malloc(pattern_count * sizeof(int));
	
	prefix_table[0] = 0;
	int i, j = 0;
	
	for (i = 1; i < pattern_count; i++) {
		int diff = (int) pattern[j] - (int) pattern[i];
		while (j > 0 && diff != 0) {
			j = prefix_table[j - 1];
		}
		
		if (diff == 0) {
			j++;
		}
		
		prefix_table[i] = j;
	}
	
	// Setting data for worker thread and calling worker threads
	pthread_t workers[MAX_NUM_OF_THREADS];
	
	i = 1;
	struct func_data *segment_input;
	clock_t start, end;
	while (i <= MAX_NUM_OF_THREADS) {
		start = clock();
		result = 0;
		segment_input = (struct func_data *) malloc(sizeof(struct func_data));
		segment_input->prefix_table = prefix_table;
		segment_input->pattern = in->pattern;
		segment_input->text = in->text;
		segment_input->start = 0;
		segment_input->count = in->count;
		int blockSize = (in->count / i);
		segment_input->blockSize = blockSize;
		// Worker Thread Creation part
		for (j = 0; j < i; j ++) {
			res[j] = 0;
			segment_input->thread_num = j + 1;
			segment_input->start = j * blockSize;  
			segment_input->end = segment_input->start + blockSize;
			
			int diff = (in->count - segment_input->end);
			if (diff < blockSize) {
				segment_input->end = segment_input->end + diff;
			}
			
			// Creating Worker Threads
			if (pthread_create(&workers[j], NULL, string_search_function, (void *) segment_input)) {
				fprintf(stderr, "Error creating worker thread %d\n", (j + 1));
				exit(-1);
			}
			
			// Joining Worker Threads
			if (pthread_join(workers[j], NULL)) {
				fprintf(stderr, "Error joining worker thread %d\n", (j + 1));
				exit(-1);
			}
		}
		
		for (j = 0; j < i; j++) {
			result += res[j];
		}
		// Output Format -- blockSize:  5768171 numOfThread:    1 matchCount: 156 runningTime:    47511
		end = clock();
		printf("blockSize:  %d\t numOfThread:    %d\t matchCount: %d\t runningTime:    %f\n", blockSize, i, result, ((double) (end - start) / CLOCKS_PER_SEC) * 1000);
		segment_input->start = 0;
		i ++;
	}
	return NULL;
}

int main(int argc, char *argv[]) {
	pthread_t master_thread;
	
	// Data struct size allocation
	struct thread_data *input;
	input = (struct thread_data *) malloc(sizeof(struct thread_data));
	input->pattern = (char *) malloc(80 * sizeof(char));
	input->text = (char *) malloc (INT_MAX * sizeof(char));
	
	// Argument match
	if (argc == 2) {
		input->pattern = argv[1];
	} else {
		printf("Error: Usage ./string_search <pattern> < input file\n");
		exit(-1);
	}
	
	// Reading the standard input and also updating total characters in input
	char ch;
	int count = 0;
	while ((ch = getchar()) != EOF) {
		input->text[count] = ch;
		count ++;
	}
	input->count = count;
	
	// Creating Manager Thread
	if (pthread_create(&master_thread, NULL, manager_thread, (void *) input)) {
		fprintf(stderr, "Error creating manager thread\n");
		exit(-1);
	}
	
	// Joining Manager Thread
	if (pthread_join(master_thread, NULL)) {
		fprintf(stderr, "Error joining manager thread\n");
		exit(-1);
	}

	return 0;
}
