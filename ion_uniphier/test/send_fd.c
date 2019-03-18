
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "send_fd.h"

int connect_un(const char *path, int *sock)
{
	int s;
	union {
		struct sockaddr ad;
		struct sockaddr_un un;
	} sock_addr;
	int result;

	if (sock == NULL) {
		return -1;
	}

	s = socket(PF_UNIX, SOCK_DGRAM, 0);
	if (sock < 0) {
		fprintf(stderr, "Failed to socket(unix, send).\n");
		return s;
	}

	memset(&sock_addr, 0, sizeof(sock_addr));
	sock_addr.un.sun_family = AF_UNIX;
	strncpy(sock_addr.un.sun_path, path, sizeof(sock_addr.un.sun_path) - 1);

	result = connect(s, &sock_addr.ad, sizeof(sock_addr));
	if (result < 0) {
		fprintf(stderr, "Failed to connect().\n");
		return result;
	}

	*sock = s;

	return 0;
}

int bind_un(const char *path, int *sock)
{
	int s;
	union {
		struct sockaddr ad;
		struct sockaddr_un un;
	} sock_addr;
	int result;

	if (sock == NULL) {
		return -1;
	}

	s = socket(PF_UNIX, SOCK_DGRAM, 0);
	if (sock < 0) {
		fprintf(stderr, "Failed to socket(unix, recv).\n");
		return s;
	}

	memset(&sock_addr, 0, sizeof(sock_addr));
	sock_addr.un.sun_family = AF_UNIX;
	strncpy(sock_addr.un.sun_path, path, sizeof(sock_addr.un.sun_path) - 1);

	result = bind(s, &sock_addr.ad, sizeof(sock_addr));
	if (result < 0) {
		perror("bind");
		fprintf(stderr, "Failed to bind().\n");
		return result;
	}

	*sock = s;

	return 0;
}

int send_fd(int sock, void *message, size_t len_message, int fd)
{
	struct iovec iov[1];
	struct msghdr msg;
	struct cmsghdr *cmsg = NULL;
	union {
		struct cmsghdr hdr;
		char cmsgbuf[CMSG_SPACE(sizeof(uint64_t))];
	} buf;
	int result;

	if (fd == -1) {
		return -1;
	}

	iov[0].iov_base = message;
	iov[0].iov_len = len_message;

	cmsg = &buf.hdr;
	cmsg->cmsg_len = CMSG_LEN(sizeof(uint64_t));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;

	memset(CMSG_DATA(cmsg), 0, sizeof(uint64_t));
	memmove(CMSG_DATA(cmsg), &fd, sizeof(int));

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_control = buf.cmsgbuf;
	msg.msg_controllen = sizeof(buf.cmsgbuf);
	msg.msg_flags = 0;

	result = sendmsg(sock, &msg, 0);
	if (result < 0) {
		fprintf(stderr, "Failed to sendmsg(fd).\n");
		return result;
	}

	return 0;
}

int recv_fd(int sock, void *message, size_t len_message, int *fd)
{
	struct iovec iov[1];
	struct msghdr msg;
	struct cmsghdr *cmsg = NULL;
	union {
		struct cmsghdr hdr;
		char cmsgbuf[CMSG_SPACE(sizeof(uint64_t))];
	} buf;
	uint64_t fd_buf;
	int result;

	if (fd == NULL) {
		return -1;
	}

	iov[0].iov_base = message;
	iov[0].iov_len = len_message;

	cmsg = &buf.hdr;

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_control = buf.cmsgbuf;
	msg.msg_controllen = sizeof(buf.cmsgbuf);
	msg.msg_flags = MSG_WAITALL;

	result = recvmsg(sock, &msg, 0);
	if (result < 0) {
		fprintf(stderr, "Failed to recvmsg(fd).\n");
		return result;
	}

	memset(&fd_buf, 0, sizeof(uint64_t));
	memmove(&fd_buf, CMSG_DATA(cmsg), sizeof(uint64_t));

	*fd = fd_buf;

	return 0;
}
