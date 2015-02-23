#include "page_manager.h"
#include <math.h>
#include <assert.h>
#include <limits.h>

static memory_config config;

void initialize_page_manager(memory_config mc) {
	int i, j;
	config = mc;

	info.physical_memory = pow(2, mc.physical_address_space);
	info.virtual_memory = pow(2, mc.virtual_address_space);
	info.page_size_in_bits = mc.page_size * BITS_PER_BYTE;
	info.page_table_count = info.virtual_memory / info.page_size_in_bits;
	info.virtual_page_number_bits_count = log2(info.page_size_in_bits);
	info.offset_bits_count = mc.virtual_address_space - info.virtual_page_number_bits_count;
	info.free_list_index = 0;
	info.free_list_count = info.physical_memory / info.page_size_in_bits;
	info.free_list = safe_malloc(info.free_list_count * sizeof(physical_page));

	// calculate physical address for each physical page
	for (i = 0; i < info.free_list_count; i++) {
		info.free_list[i].start_address = i * info.page_size_in_bits;
		info.free_list[i].end_address = info.free_list[i].start_address + (info.page_size_in_bits - 1);
	}

	// Verify that the physical page address for last page is correct
	assert(info.free_list[i - 1].end_address == info.physical_memory - 1);

	// initialize per process page table
	info.processes = safe_malloc(mc.processes * sizeof(process));

	for (i = 0; i < mc.processes; i++) {
		info.processes[i].id = i;
		info.processes[i].page_table = safe_malloc(info.page_table_count * sizeof(virtual_page));

		// safe_malloc calls memset with 0, verify that valid member is equal to zero
		for (j = 0; j < info.page_table_count; j++) assert(info.processes[i].page_table[j].valid == 0);
	}

	write_log2("memory config:");
	LOG_MSG("%d physical address space", mc.physical_address_space);
	LOG_MSG("%d virtual address space", mc.virtual_address_space);
	LOG_MSG("%d processes", mc.processes);
	LOG_MSG("%d page size in bytes and %d page size in bits", mc.page_size, info.page_size_in_bits);

	write_log2("system info:");
	LOG_MSG("%d physical memory", info.physical_memory);
	LOG_MSG("%d virtual memory", info.virtual_memory);
	LOG_MSG("%d free physical pages", info.free_list_count);
	LOG_MSG("%d current free physical page", info.free_list_index);
	LOG_MSG("%d offset bits count", info.offset_bits_count);
	LOG_MSG("%d VPN bits count", info.virtual_page_number_bits_count);
	LOG_MSG("%d entries in page table per process", info.page_table_count);
}

access_result access_memory(unsigned int pid, unsigned int virtual_address) {
	access_result result;
	unsigned int i, vpn, offset, lru_vpn, time_diff;
	time_t min_time;

	assert(pid < config.processes);
	assert(pid == info.processes[pid].id);

	offset = get_bits(virtual_address, 0, info.offset_bits_count);
	vpn = get_bits(virtual_address, info.offset_bits_count, info.virtual_page_number_bits_count);

	assert(vpn < info.page_table_count);
	assert(offset < info.page_size_in_bits);
	result.page_fault = !info.processes[pid].page_table[vpn].valid;

	if (result.page_fault) {
		if (info.free_list_index < info.free_list_count) {
			// a free physical page is available
			result.physical_page_number = info.free_list_index++;
		} else {
			min_time = time(NULL);
			lru_vpn = INT_MAX;
			for (i = 0; i < info.page_table_count; i++) {
				if (!info.processes[pid].page_table[i].valid) {
					continue;
				}

				time_diff = difftime(min_time, info.processes[pid].page_table[i].last_accessed);
				if (time_diff > 0) {
					lru_vpn = i;
					min_time = info.processes[pid].page_table[i].last_accessed;
				}
			}

			assert(lru_vpn != INT_MAX);
			result.physical_page_number = info.processes[pid].page_table[lru_vpn].physical_page;
			info.processes[pid].page_table[lru_vpn].valid = false;
		}

		info.processes[pid].page_table[vpn].valid = true;
		info.processes[pid].page_table[vpn].physical_page = result.physical_page_number;

	} else {
		assert(info.processes[pid].page_table[vpn].valid);
		result.physical_page_number = info.processes[pid].page_table[vpn].physical_page;
	}

	info.processes[pid].page_table[vpn].last_accessed = time(NULL);

	result.virtual_page_number = vpn;
	result.physical_address = info.free_list[result.physical_page_number].start_address + offset;
	assert(result.physical_address <= info.free_list[result.physical_page_number].end_address);

	return result;
}

