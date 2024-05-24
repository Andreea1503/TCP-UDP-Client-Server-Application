#ifndef __HELPER_H__
#define __HELPER_H__

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUF_LEN 1600
#define ARG_NO 4

int sockfd, ret, n;
struct sockaddr_in servaddr;
char buffer[BUF_LEN];
fd_set read_fds, tmp_fds;


#endif // __HELPER_H__