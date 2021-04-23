/*
 * Loader Implementation
 *
 * 2018, Operating Systems
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/mman.h>
#include <signal.h>
#include <unistd.h>

#include <sys/stat.h>
#include <fcntl.h>

#include "exec_parser.h"

#define SEGMENT_INDEX_ERROR -1

static so_exec_t *exec;
static struct sigaction old_action;
int fd;
static int pageSize;


typedef struct page_info {
	int mapped;
}page_info_t;

int getSegmentPositionOfAddr(char *addr)
{
	int index = SEGMENT_INDEX_ERROR;

	for(int i = 0 ; i < exec->segments_no ; i++) {
		if (addr >= (char*)(exec->segments[i].vaddr) && addr <= (char*)(exec->segments[i].vaddr + exec->segments[i].mem_size)) {
			index = i;
			break;
		}
	}
	return index;
}

void initSegmentData(int segment_index)
{
	int number_of_pages;

	number_of_pages = exec->segments[segment_index].mem_size / pageSize;
	if (exec->segments[segment_index].mem_size % pageSize != 0) {
	}
		number_of_pages += 1;
	exec->segments[segment_index].data = calloc(number_of_pages, sizeof(page_info_t));
	printf("no of pages: [%d]\n", number_of_pages);
}

char* call_mmap(int segment_index, int page) {
	return mmap((void*) exec->segments[segment_index].vaddr +
			pageSize * page,
			pageSize,
			exec->segments[segment_index].perm,
			MAP_FIXED | MAP_PRIVATE,
			fd,
			exec->segments[segment_index].offset + pageSize * page);
}

void legal_page_fault(int segment_index, int page) {
	char *mmap_addr;
	unsigned int mem_size = exec->segments[segment_index].mem_size;
	unsigned int file_size = exec->segments[segment_index].file_size;
	int size_to_write = pageSize;


	printf("mapez [%d] -> [%d]\n", exec->segments[segment_index].vaddr +
			pageSize * page, exec->segments[segment_index].vaddr +
			pageSize * page + pageSize);

	mmap_addr = call_mmap(segment_index, page);
	if (mmap_addr == MAP_FAILED) {
		printf("map failed\n");
	}
	// printf("res: [%d]\n", res);
	((page_info_t*)exec->segments[segment_index].data)[page].mapped = 1;

	if (mem_size > file_size) {
		size_to_write -= (file_size - page * pageSize);
	}
}

static void segv_handler(int signum, siginfo_t *info, void *context)
{
	char *addr = info->si_addr;
	int segment_index = SEGMENT_INDEX_ERROR;
	int page,  number_of_pages;
	page_info_t *data;


	segment_index = getSegmentPositionOfAddr(addr);
	if (segment_index == SEGMENT_INDEX_ERROR) {
		printf("segfault la [%d]\n", addr);
		old_action.sa_sigaction(signum, info, context);
	}

	page = ((int)addr - exec->segments[segment_index].vaddr) / pageSize;	
	if (exec->segments[segment_index].data == NULL) {
		initSegmentData(segment_index);
	}
	printf("segment [%d]", segment_index);
	printf("page [%d]", page);
	data = exec->segments[segment_index].data;
	if (data[page].mapped == 1) {
		old_action.sa_sigaction(signum, info, context);
	} else {
		legal_page_fault(segment_index, page);
	}
	
	
}


static void set_signal(void)
{
	struct sigaction action;
	int rc;

	action.sa_sigaction = segv_handler;
	sigemptyset(&action.sa_mask);
	sigaddset(&action.sa_mask, SIGSEGV);
	action.sa_flags = SA_SIGINFO;

	rc = sigaction(SIGSEGV, &action, &old_action);
}


int so_init_loader(void)
{
	pageSize = getpagesize();
	set_signal();

	return -1;
}

int so_execute(char *path, char *argv[])
{
	fd = open(path, O_RDONLY, 0744);

	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	so_start_exec(exec, argv);

	return -1;
}
