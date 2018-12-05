#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

int sock_create(char *ip, unsigned short port)
{
	int sockfd;
	int yes = 1;
	struct sockaddr_in addr;
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
		perror("reuseaddr");
                exit(1);
	}

	/* init servaddr */
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip);
	addr.sin_port = htons(port);
	/* bind address and port to socket */
	if(bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		perror("bind error");
		exit(1);
	}
	return sockfd;
}

int readn(int fd, void *buf, int n)
{
        int ret;
        int last = n;
        int p = 0;
        while(last) {
                ret = read(fd, buf + p, last);
                if(ret < 0)
			return ret;
		if(ret == 0)
			return n - last;
                last -= ret;
                p += ret;
        }
	return n;
}

#define LEN 4096
int main(int argc, char **argv)
{
	unsigned char buf[LEN];
	int fd = sock_create("127.0.0.1", 53);
	struct sockaddr_in dnsaddr;
	bzero(&dnsaddr, sizeof(dnsaddr));
	dnsaddr.sin_family = AF_INET;
	dnsaddr.sin_addr.s_addr = inet_addr("8.8.8.8");
	dnsaddr.sin_port = htons(53);

	signal(SIGCHLD, SIG_IGN);

	while(1) {
		struct sockaddr_in peer_addr;
		socklen_t slen = sizeof(struct sockaddr_in);
		int n, ret;

		n = recvfrom(fd, buf + 2, LEN - 2, 0, (struct sockaddr *)&peer_addr, &slen);
		if (n < 0)
			break;

		if (fork() == 0) {
			int fd2 = socket(AF_INET, SOCK_STREAM, 0);
			if (fd2 < 0) {
				perror("socket");
				exit(1);
			}

			ret = connect(fd2, (struct sockaddr *)&dnsaddr, sizeof(struct sockaddr_in));
			if (ret < 0) {
				perror("connect");
				close(fd2);
				exit(1);
			}

			buf[0] = (n >> 8);
			buf[1] = n & 0xff;
			ret = write(fd2, buf, n + 2);
			if (n + 2 != ret) {
				perror("write");
				close(fd2);
				exit(1);
			}

			n = readn(fd2, buf, 2);
			if (n != 2) {
				perror("read1");
				close(fd2);
				exit(1);
			}

			n = (buf[0] << 8) | buf[1];
			if (n <= 0 || n > LEN) {
				close(fd2);
				exit(1);
			}

			ret = readn(fd2, buf, n);
			if (n != ret) {
				perror("read2");
				close(fd2);
				exit(1);
			}

			sendto(fd, buf, n, 0, (struct sockaddr *)&peer_addr, slen);
			close(fd2);
			exit(0);
		}
	}
	close(fd);
	return 0;
}
