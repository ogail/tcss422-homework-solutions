#ifndef _PAGE_MANAGER_H_
#define _PAGE_MANAGER_H_

#include "utility.h"
#include <time.h>

typedef struct {
	unsigned int physical_address_space;  /* in bits, between 2 and 32 (inclusive) */
	unsigned int virtual_address_space;   /* in bits, between 2 and 32 (inclusive) */
	unsigned int page_size;               /* in bytes, a power of 2 */
	unsigned int processes;               /* positive */
} memory_config;

typedef struct {
	unsigned int virtual_page_number;
	unsigned int physical_page_number;
	unsigned int physical_address;
	char page_fault;                    /* zero means false, non-zero means true */
} access_result;

typedef struct {
	unsigned int start_address;
	unsigned int end_address;
} physical_page;

typedef struct {
	byte valid;
	unsigned int physical_page;
	time_spec last_accessed;
} virtual_page;

typedef struct {
	unsigned int id;
	virtual_page * page_table;
} process;

typedef struct {
	unsigned int physical_memory;
	unsigned int virtual_memory;
	unsigned int page_size;
	unsigned int free_list_count;
	unsigned int free_list_index;
	unsigned int virtual_page_number_bits_count;
	unsigned int offset_bits_count;
	unsigned int page_table_count;
	physical_page * free_list;
	process * processes;
} os_memory_info;

os_memory_info info;

void initialize_page_manager(memory_config mc);

void destroy_page_manager();

access_result access_memory(unsigned int pid, unsigned int virtual_address);

#endif
