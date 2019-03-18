
#ifndef SEND_FD__
#define SEND_FD__

int connect_un(const char *path, int *sock);
int bind_un(const char *path, int *sock);
int send_fd(int sock, void *message, size_t len_message, int fd);
int recv_fd(int sock, void *message, size_t len_message, int *fd);

#endif //SEND_FD__
