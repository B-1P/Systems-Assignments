/* Header file for wrapper functions */

int Read(int fd, char *buffer, size_t size);

int Write(int fd, char *buffer, size_t size);

int Accept(int socket, struct sockaddr *address, socklen_t *address_len);

void Bind(int fd, const struct sockaddr *sa, socklen_t salen);

void Listen(int fd, int backlog);

int Socket(int family, int type, int protocol);

int Select(int nfds, fd_set *rfds, fd_set *wfds, fd_set *efds, struct timeval *tv);

void Close(int fds);

void *Malloc(size_t size);