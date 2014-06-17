#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>


/*
 * Wrapper functions to handle errors.
 */

int Read(int fd, char *buffer, size_t size) {
    int result = read(fd, buffer, size);
    if (result == -1) {
        perror("Read Error");
        exit(-1);
    } else {
        return result;
    }
}

int Write(int fd, char *buffer, size_t size) {
    int result = write(fd, buffer, size);
    if (result == -1) {
        perror("Write error");
        exit(-1);
    } else {
        return result;
    }
}

int Accept(int socket, struct sockaddr *address, socklen_t *address_len) {
    int cfd;
    if ((cfd = accept(socket, address, address_len)) < 0) {
        perror("Aceept Error");
        exit(1);
    }
    return cfd;
}

void Bind(int fd, const struct sockaddr *sa, socklen_t salen) {
    if (bind(fd, sa, salen) < 0) {
        perror("Bind error");
        exit(1);
    }
}

void Listen(int fd, int backlog) {
    if (listen(fd, backlog) < 0) {
        perror("listen error");
        exit(1);
    }
}

int Socket(int family, int type, int protocol) {
    int n;
    if ( (n = socket(family, type, protocol)) < 0) {
        perror("socket error");
        exit(1);
    }
    return (n);
}

int Select(int nfds, fd_set *rfds, fd_set *wfds, fd_set *efds, struct timeval *tv) {
    int ret;
    ret = select(nfds, rfds, wfds, efds, tv);
    if (ret < 0) {
        perror("Select Eroor");
        exit(-1);
    }
    return ret;
}

void Close(int fds) {
    if (close(fds) < 0) {
        perror("Error closing socket");
        exit(-1);
    }
}

void *Malloc(size_t size) {
    void *p;
    p = malloc(size);
    if (!p) {
        perror("Error freeing memory");
        exit(-1);
    }

    return p;
}

void FClose(FILE *fp){
    if(fclose(fp)!=0){
        perror("Error closing client file");
        exit(-1);
    }
}
