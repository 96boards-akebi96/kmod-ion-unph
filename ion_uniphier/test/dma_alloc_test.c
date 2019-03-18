
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <asm/ion.h>
#include <asm/ion_uniphier.h>

#include "send_fd.h"

#define ION_DEVNAME      "/dev/ion"
#define UNIX_SOCKNAME    "/tmp/un.sock"

void print_speed(struct timeval start, struct timeval end, size_t total)
{
	struct timeval elapse;
	uint64_t usec, speed;

	timersub(&end, &start, &elapse);

	usec = elapse.tv_sec * 1000000 + elapse.tv_usec;
	speed = total / (usec / 1000);
	printf("  done, %d[KB], %d.%03d[ms], %d.%03d[MB/s]\n",
		(int)total / 1024,
		(int)elapse.tv_sec, (int)elapse.tv_usec / 1000,
		(int)speed / 1000, (int)speed % 1000);
}

void burst_read8(uint8_t *addr, size_t size, int rep)
{
	struct timeval start, end;
	volatile int sum;
	size_t i, j;

	sum = 0;
	gettimeofday(&start, NULL);
	for (j = 0; j < rep; j++) {
		for (i = 0; i < size / sizeof(*addr); i++) {
			sum += addr[i];
		}
	}
	gettimeofday(&end, NULL);
	//printf("sum:%d\n", sum);
	print_speed(start, end, rep * size);
}

void burst_read16(uint16_t *addr, size_t size, int rep)
{
	struct timeval start, end;
	volatile int sum;
	size_t i, j;

	sum = 0;
	gettimeofday(&start, NULL);
	for (j = 0; j < rep; j++) {
		for (i = 0; i < size / sizeof(*addr); i++) {
			sum += addr[i];
		}
	}
	gettimeofday(&end, NULL);
	//printf("sum:%d\n", sum);
	print_speed(start, end, rep * size);
}

void burst_read32(uint32_t *addr, size_t size, int rep)
{
	struct timeval start, end;
	volatile int sum;
	size_t i, j;

	sum = 0;
	gettimeofday(&start, NULL);
	for (j = 0; j < rep; j++) {
		for (i = 0; i < size / sizeof(*addr); i++) {
			sum += addr[i];
		}
	}
	gettimeofday(&end, NULL);
	//printf("sum:%d\n", sum);
	print_speed(start, end, rep * size);
}

void burst_read64(uint64_t *addr, size_t size, int rep)
{
	struct timeval start, end;
	volatile int sum;
	size_t i, j;

	sum = 0;
	gettimeofday(&start, NULL);
	for (j = 0; j < rep; j++) {
		for (i = 0; i < size / sizeof(*addr); i++) {
			sum += addr[i];
		}
	}
	gettimeofday(&end, NULL);
	//printf("sum:%d\n", sum);
	print_speed(start, end, rep * size);
}

void burst_write8(uint8_t *addr, size_t size, int rep)
{
	struct timeval start, end;
	size_t i, j;

	gettimeofday(&start, NULL);
	for (j = 0; j < rep; j++) {
		for (i = 0; i < size / sizeof(*addr); i++) {
			addr[i] = i + j;
		}
	}
	gettimeofday(&end, NULL);
	print_speed(start, end, rep * size);
}

void burst_write16(uint16_t *addr, size_t size, int rep)
{
	struct timeval start, end;
	size_t i, j;

	gettimeofday(&start, NULL);
	for (j = 0; j < rep; j++) {
		for (i = 0; i < size / sizeof(*addr); i++) {
			addr[i] = i + j;
		}
	}
	gettimeofday(&end, NULL);
	print_speed(start, end, rep * size);
}

void burst_write32(uint32_t *addr, size_t size, int rep)
{
	struct timeval start, end;
	size_t i, j;

	gettimeofday(&start, NULL);
	for (j = 0; j < rep; j++) {
		for (i = 0; i < size / sizeof(*addr); i++) {
			addr[i] = i + j;
		}
	}
	gettimeofday(&end, NULL);
	print_speed(start, end, rep * size);
}

void burst_write64(uint64_t *addr, size_t size, int rep)
{
	struct timeval start, end;
	size_t i, j;

	gettimeofday(&start, NULL);
	for (j = 0; j < rep; j++) {
		for (i = 0; i < size / sizeof(*addr); i++) {
			addr[i] = i + j;
		}
	}
	gettimeofday(&end, NULL);
	print_speed(start, end, rep * size);
}

void bench(char *addr, size_t size, int rep)
{
	uint8_t *addr8 = (uint8_t *)addr;
	uint16_t *addr16 = (uint16_t *)addr;
	uint32_t *addr32 = (uint32_t *)addr;
	uint64_t *addr64 = (uint64_t *)addr;
	struct timeval start, end;
	size_t j;

	printf("8bit read\n");
	burst_read8(addr8, size, rep);

	printf("16bit read\n");
	burst_read16(addr16, size, rep);

	printf("32bit read\n");
	burst_read32(addr32, size, rep);

	printf("64bit read\n");
	burst_read64(addr64, size, rep);

	printf("8bit write\n");
	burst_write8(addr8, size, rep);

	printf("16bit write\n");
	burst_write16(addr16, size, rep);

	printf("32bit write\n");
	burst_write32(addr32, size, rep);

	printf("64bit write\n");
	burst_write64(addr64, size, rep);

	printf("memset\n");
	gettimeofday(&start, NULL);
	for (j = 0; j < rep; j++) {
		memset(addr, j + 100, size);
	}
	gettimeofday(&end, NULL);
	print_speed(start, end, rep * size);

	printf("memcpy\n");
	gettimeofday(&start, NULL);
	for (j = 0; j < rep * 2; j++) {
		memcpy(addr + size / 2, addr, size / 2);
	}
	gettimeofday(&end, NULL);
	print_speed(start, end, rep * size);
}

int main(int argc, char *argv[])
{
	int fd_ion = -1;
	int sock = -1;
	int heap_id;
	struct ion_allocation_data alloc_buf;
	struct ion_fd_data share_buf;
	struct ion_handle_data free_buf;
	char *addr = NULL;
	struct ion_custom_data ioctl_buf;
	struct ion_uniphier_virt_to_phys_data v2p_buf;
	int rep;
	int result = -EIO;

	heap_id = ION_HEAP_ID_MEDIA;
	rep = 1;

	printf("open\n");
	getchar();

	fd_ion = open(ION_DEVNAME, O_RDWR);
	if (fd_ion == -1) {
		result = errno;
		fprintf(stderr, "Failed to open(ion).\n");
		goto err_out;
	}


	printf("alloc\n");
	getchar();

	memset(&alloc_buf, 0, sizeof(alloc_buf));
	alloc_buf.len = 0x2000000;
	alloc_buf.align = 0x1000;
	alloc_buf.heap_id_mask = 0x1 << heap_id;
	alloc_buf.flags = 0;
	result = ioctl(fd_ion, ION_IOC_ALLOC, &alloc_buf);
	if (result != 0) {
		result = errno;
		fprintf(stderr, "Failed to ioctl(alloc).\n");
		goto err_out;
	}


	printf("export\n");
	getchar();

	memset(&share_buf, 0, sizeof(share_buf));
	share_buf.handle = alloc_buf.handle;
	result = ioctl(fd_ion, ION_IOC_SHARE, &share_buf);
	if (result != 0) {
		result = errno;
		fprintf(stderr, "Failed to ioctl(share).\n");
		goto err_out;
	}
	printf("export fd:%d\n", share_buf.fd);


	printf("mmap\n");
	getchar();

	addr = mmap(NULL, alloc_buf.len, PROT_READ | PROT_WRITE,
		MAP_SHARED, share_buf.fd, 0);
	//addr = malloc(alloc_buf.len);
	if (addr == MAP_FAILED) {
		result = errno;
		fprintf(stderr, "Failed to mmap().\n");
		goto err_out;
	}
	memset(addr, 3, alloc_buf.len);
	printf("mmap addr:%p-%p\n", addr, addr + alloc_buf.len);

	printf("waiting...\n");
	sleep(3);
	printf("done.\n");


	printf("benchmark\n");
	getchar();
	bench(addr, alloc_buf.len, rep);


	printf("get physical address\n");
	getchar();

	memset(&ioctl_buf, 0, sizeof(ioctl_buf));
	ioctl_buf.cmd = ION_UNIP_IOC_VIRT_TO_PHYS;
	ioctl_buf.arg = (unsigned long)&v2p_buf;
	memset(&v2p_buf, 0, sizeof(v2p_buf));
	v2p_buf.handle = alloc_buf.handle;
	v2p_buf.virt = (uintptr_t)addr;
	v2p_buf.len = alloc_buf.len;
	//phys_buf.phys;
	result = ioctl(fd_ion, ION_IOC_CUSTOM, &ioctl_buf);
	if (result != 0) {
		result = errno;
		fprintf(stderr, "Failed to ioctl(custom, virt_to_phys).\n");
		goto err_out;
	}
	printf("get phys:0x%08lx, len:%d, cont:%d, virt:%p\n",
		(long)v2p_buf.phys, (int)v2p_buf.len, v2p_buf.cont, addr);


	printf("connect/send\n");
	getchar();

	result = connect_un(UNIX_SOCKNAME, &sock);
	if (result != 0) {
		fprintf(stderr, "Failed to connect_un().\n");
		goto err_out;
	}

	result = send_fd(sock, "hoge", 5, share_buf.fd);
	if (result != 0) {
		fprintf(stderr, "Failed to send_fd().\n");
		goto err_out;
	}


	printf("munmap\n");
	getchar();

	result = munmap(addr, alloc_buf.len);
	if (result != 0) {
		result = errno;
		fprintf(stderr, "Failed to munmap().\n");
		goto err_out;
	}


	printf("free\n");
	getchar();

	memset(&free_buf, 0, sizeof(free_buf));
	free_buf.handle = alloc_buf.handle;
	result = ioctl(fd_ion, ION_IOC_FREE, &free_buf);
	if (result != 0) {
		result = errno;
		fprintf(stderr, "Failed to ioctl(free).\n");
		goto err_out;
	}


	printf("close\n");
	getchar();

	//Success
	result = 0;

err_out:
	if (sock != -1) {
		close(sock);
		sock = -1;
	}

	if (fd_ion != -1) {
		close(fd_ion);
		fd_ion = -1;
	}

	return result;
}
