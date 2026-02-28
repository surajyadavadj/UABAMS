#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <stdint.h>
#include <stddef.h>

/* Connect to host:port.  Returns socket fd on success, -1 on error. */
int tcp_connect(const char *host, uint16_t port);

/* Send buf[0..len-1] on fd.  Returns bytes sent, -1 on error. */
int tcp_send(int fd, const void *buf, size_t len);

/* Close the socket. */
void tcp_close(int fd);

#endif
