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
#include "my_die.h"

#define SEGMENT_INDEX_ERROR -1

static so_exec_t *exec;
static struct sigaction old_action;
int fd;
static int pageSize;


/* keeps info about the mapping situation of a page*/
typedef struct page_info {
	int mapped;
}page_info_t;


/* get the index  of the segment that contains the address, or error*/
int getSegmentPositionOfAddr(char *addr)
{
	int index = SEGMENT_INDEX_ERROR, i;

	/* loop through all the segments*/
	for (i = 0 ; i < exec->segments_no ; i++) {
		/* if the segment contains the address*/
		if (addr >= (char *)(exec->segments[i].vaddr) &&
			addr <= (char *)(exec->segments[i].vaddr +
			exec->segments[i].mem_size)) {
			index = i;
			break;
		}
	}
	return index;
}


/* init data structure that contains mapping information*/
void initSegmentData(int segment_index)
{
	int number_of_pages;

	number_of_pages = exec->segments[segment_index].mem_size / pageSize;

	/*every segments has a data field*/
	exec->segments[segment_index].data =
		calloc(number_of_pages, sizeof(page_info_t));
}


/* the mmap call which use a file descriptor*/
char *call_mmap_on_file(int segment_index, int page, int size)
{
	return mmap((void *) exec->segments[segment_index].vaddr +
			pageSize * page,
			pageSize,
			exec->segments[segment_index].perm,
			MAP_FIXED | MAP_PRIVATE,
			fd,
			exec->segments[segment_index].offset +
				pageSize * page);
}


/* the mmap call used for bss to map anonymously*/
char *call_anonymous_mmap(int segment_index, int page)
{
	return mmap((void *) exec->segments[segment_index].vaddr +
			pageSize * page,
			pageSize,
			exec->segments[segment_index].perm,
			MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS,
			-1,
			0);
}


/* manage the page fault on a valid address*/
void legal_page_fault(int segment_index, int page, char *addr)
{
	char *mmap_addr;
	size_t mem_size = exec->segments[segment_index].mem_size;
	size_t file_size = exec->segments[segment_index].file_size;
	size_t zero_mem_size = 0;
	int start_page_offset = pageSize * page;
	int end_page_offset = pageSize * (page + 1) - 1, res;

	/* there is no bss, so we map the entire page*/
	if (mem_size == file_size) {
		mmap_addr = call_mmap_on_file(segment_index, page, pageSize);
		DIE(mmap_addr == MAP_FAILED, "map failed");
	}

	/* there is bss*/
	if (mem_size > file_size) {
		/* the page is pointed by the file size*/
		if (file_size > start_page_offset
			&& file_size <= end_page_offset) {
			/* the size which will be filled with zeros*/
			zero_mem_size = end_page_offset - file_size;
			mmap_addr =
				call_mmap_on_file(segment_index, page,
						file_size - start_page_offset);
			DIE(mmap_addr == MAP_FAILED, "map failed");
			/* change the protection mode to write*/
			res = mprotect(mmap_addr, pageSize, PROT_WRITE);
			DIE(res == -1, "mprotect failed");
			/* fill the bss section with zeros*/
			memset(mmap_addr + file_size - start_page_offset,
				0,
				zero_mem_size);
			/* set the protection mode back*/
			mprotect(mmap_addr,
				pageSize,
				exec->segments[segment_index].perm);
			DIE(res == -1, "mprotect failed");
		/* the page is outside file size => bss*/
		} else if (file_size <= start_page_offset) {
			mmap_addr = call_anonymous_mmap(segment_index, page);
			DIE(mmap_addr == MAP_FAILED, "map failed");
		/* the page is inside the file size*/
		} else {
			mmap_addr = call_mmap_on_file(segment_index,
							page,
							pageSize);
			DIE(mmap_addr == MAP_FAILED, "map failed");
		}
	}
	/* we need to know that the page was mapped*/
	((page_info_t *)exec->segments[segment_index].data)[page].mapped = 1;
}

/* the new segv handler used for SIGSEGV*/
static void segv_handler(int signum, siginfo_t *info, void *context)
{
	char *addr = info->si_addr;
	int segment_index = SEGMENT_INDEX_ERROR;
	int page;
	page_info_t *data;

	/* calculate the index of the segment*/
	segment_index = getSegmentPositionOfAddr(addr);
	/* invalid address => default handler which will cause a seg fault*/
	if (segment_index == SEGMENT_INDEX_ERROR)
		old_action.sa_sigaction(signum, info, context);

	/* calculate the page number*/
	page = ((int)addr - exec->segments[segment_index].vaddr) / pageSize;
	/* init the data structure that keeps the mapping info*/
	if (exec->segments[segment_index].data == NULL)
		initSegmentData(segment_index);
	data = exec->segments[segment_index].data;

	/* if the page is already mapped => run the default handler*/
	if (data[page].mapped == 1)
		old_action.sa_sigaction(signum, info, context);
	/* the page is not already mapped, so we have to map it*/
	else
		legal_page_fault(segment_index, page, addr);
}


static void set_signal(void)
{
	struct sigaction action;
	int rc;

	action.sa_sigaction = segv_handler;
	sigemptyset(&action.sa_mask);
	sigaddset(&action.sa_mask, SIGSEGV);
	action.sa_flags = SA_SIGINFO;

	/* set the new action and keep the old action*/
	rc = sigaction(SIGSEGV, &action, &old_action);
	DIE(rc == -1, "sigaction failed");
}


int so_init_loader(void)
{
	pageSize = getpagesize();
	/* initiates the new segv handler*/
	set_signal();

	return -1;
}

int so_execute(char *path, char *argv[])
{
	/* open the executable file*/
	fd = open(path, O_RDONLY, 0744);

	DIE(fd == -1, "open failed");
	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	so_start_exec(exec, argv);

	return -1;
}
