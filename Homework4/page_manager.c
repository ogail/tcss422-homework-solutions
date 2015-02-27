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
	info.page_size = mc.page_size;
	info.page_table_count = info.virtual_memory / info.page_size;
	info.virtual_page_number_bits_count = log2(info.page_table_count);
	info.offset_bits_count = mc.virtual_address_space - info.virtual_page_number_bits_count;
	info.free_list_index = 0;
	info.free_list_count = info.physical_memory / info.page_size;
	info.free_list = safe_malloc(info.free_list_count * sizeof(physical_page));

	// calculate physical address for each physical page
	for (i = 0; i < info.free_list_count; i++) {
		info.free_list[i].start_address = i * info.page_size;
		info.free_list[i].end_address = info.free_list[i].start_address + (info.page_size - 1);
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

	write_log2("memory config:\n");
	LOG_MSG("%d physical address space", mc.physical_address_space);
	LOG_MSG("%d virtual address space", mc.virtual_address_space);
	LOG_MSG("%d processes", mc.processes);
	LOG_MSG("%d page size in bytes and %d page size in bits", mc.page_size, info.page_size);

	write_log2("system info:\n");
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
	unsigned int i, vpn, offset, lru_vpn;
	time_spec min_time;

	assert(pid < config.processes);
	assert(pid == info.processes[pid].id);

	LOG_MSG("Accessing virtual address %d for process %d", virtual_address, pid);

	offset = get_bits(virtual_address, 0, info.offset_bits_count);
	vpn = get_bits(virtual_address, info.offset_bits_count, info.virtual_page_number_bits_count);

	LOG_MSG("VPN %d and offset %d", vpn, offset);

	assert(vpn < info.page_table_count);
	assert(offset < info.page_size);
	result.page_fault = !info.processes[pid].page_table[vpn].valid;

	if (result.page_fault) {
		if (info.free_list_index < info.free_list_count) {
			// a free physical page is available
			result.physical_page_number = info.free_list_index++;
		} else {
			time_now(&min_time);
			lru_vpn = INT_MAX;
			LOG_MSG("Finding LUR page for vpn %d with initial time microsec %d-%s", vpn, min_time.tv_nsec, ctime(&min_time.tv_sec));

			for (i = 0; i < info.page_table_count; i++) {
				if (!info.processes[pid].page_table[i].valid) {
					continue;
				}

				if (smaller_than(info.processes[pid].page_table[i].last_accessed, min_time)) {
					LOG_MSG("Candidate LUR page for vpn %d is %d", vpn, i);
					lru_vpn = i;
					min_time = info.processes[pid].page_table[i].last_accessed;
				}
			}

			LOG_MSG("Found LUR page %d for VPN %d", lru_vpn, vpn);
			assert(lru_vpn != INT_MAX);
			result.physical_page_number = info.processes[pid].page_table[lru_vpn].physical_page;
			info.processes[pid].page_table[lru_vpn].valid = false;
		}

		info.processes[pid].page_table[vpn].valid = true;
		info.processes[pid].page_table[vpn].physical_page = result.physical_page_number;
		LOG_MSG("VPN %d is allocated and mapped with physical page %d", vpn, result.physical_page_number);

	} else {
		assert(info.processes[pid].page_table[vpn].valid);
		result.physical_page_number = info.processes[pid].page_table[vpn].physical_page;
	}

	time_now(&info.processes[pid].page_table[vpn].last_accessed);

	result.virtual_page_number = vpn;
	result.physical_address = info.free_list[result.physical_page_number].start_address + offset;
	assert(result.physical_address <= info.free_list[result.physical_page_number].end_address);

	LOG_MSG("Access is done, the physical address is %d", result.physical_address);
	LOG_MSG("Access time is microsec %d-%s",
			info.processes[pid].page_table[vpn].last_accessed.tv_nsec,
			ctime(&info.processes[pid].page_table[vpn].last_accessed.tv_sec));

	return result;
}

void destroy_page_manager() {
	int i;

	safe_free(info.free_list);

	for (i = 0; i < config.processes; i++) {
		safe_free(info.processes[i].page_table);
	}

	safe_free(info.processes);

	assert(allocated_chunks == 0);
	write_log2("page_manager destroyed!\n");
}
