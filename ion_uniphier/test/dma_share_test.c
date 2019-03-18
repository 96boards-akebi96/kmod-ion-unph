
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
#include <asm/ion.h>
#include <asm/ion_uniphier.h>

#include "send_fd.h"

#define ION_DEVNAME      "/dev/ion"
#define UNIX_SOCKNAME    "/tmp/un.sock"

int main(int argc, char *argv[])
{
	int fd_ion = -1;
	int sock = -1;
	int fd_buf = -1;
	char buf[16];
	struct ion_fd_data share_buf;
	struct ion_handle_data free_buf;
	struct ion_custom_data ioctl_buf;
	struct ion_uniphier_virt_to_phys_data v2p_buf;
	off_t len_buf;
	char *addr = NULL;
	int result = -EIO;

	printf("open\n");
	getchar();

	fd_ion = open(ION_DEVNAME, O_RDWR);
	if (fd_ion == -1) {
		result = errno;
		fprintf(stderr, "Failed to open(ion).\n");
		goto err_out;
	}


	printf("connect/recv\n");
	getchar();

	result = bind_un(UNIX_SOCKNAME, &sock);
	if (result != 0) {
		fprintf(stderr, "Failed to bind_un().\n");
		goto err_out;
	}

	result = recv_fd(sock, buf, sizeof(buf), &fd_buf);
	if (result != 0) {
		fprintf(stderr, "Failed to recv_fd().\n");
		goto err_out;
	}
	printf("recv buf:%s, fd:%d\n", buf, fd_buf);


	printf("import\n");
	getchar();

	memset(&share_buf, 0, sizeof(share_buf));
	share_buf.fd = fd_buf;
	result = ioctl(fd_ion, ION_IOC_IMPORT, &share_buf);
	if (result != 0) {
		result = errno;
		fprintf(stderr, "Failed to ioctl(import).\n");
		goto err_out;
	}


	printf("lseek\n");
	getchar();
	len_buf = lseek(fd_buf, 0, SEEK_END);
	if (len_buf == (off_t)-1) {
		result = errno;
		fprintf(stderr, "Failed to lseek(SEEK_END).\n");
		goto err_out;
	}
	printf("len buf:%d\n", (int)len_buf);


	printf("mmap\n");
	getchar();
	addr = mmap(NULL, len_buf, PROT_READ | PROT_WRITE,
		    MAP_SHARED, share_buf.fd, 0);

	if (addr == MAP_FAILED) {
		result = errno;
		fprintf(stderr, "Failed to mmap().\n");
		goto err_out;
	}

	printf("get physical address\n");
	getchar();

	memset(&ioctl_buf, 0, sizeof(ioctl_buf));
	ioctl_buf.cmd = ION_UNIP_IOC_VIRT_TO_PHYS;
	ioctl_buf.arg = (unsigned long)&v2p_buf;
	memset(&v2p_buf, 0, sizeof(v2p_buf));
	//v2p_buf.handle = alloc_buf.handle;
	v2p_buf.virt = (uintptr_t)addr;
	v2p_buf.len =  len_buf;
	//phys_buf.phys;
	result = ioctl(fd_ion, ION_IOC_CUSTOM, &ioctl_buf);
	if (result != 0) {
		result = errno;
		fprintf(stderr, "Failed to ioctl(custom, virt_to_phys).\n");
		goto err_out;
	}
	printf("get phys:0x%08lx, len:%d, cont:%d, virt:%p\n",
	       (long)v2p_buf.phys, (int)v2p_buf.len, v2p_buf.cont, addr);
	

	printf("free\n");
	getchar();

	memset(&free_buf, 0, sizeof(free_buf));
	free_buf.handle = share_buf.handle;
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
