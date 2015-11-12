/* udp-broadcast-client.c
 * udp datagram client
 * Get datagram stock market quotes from UDP broadcast:
 * see below the step by step explanation
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

extern int mkaddr(void *addr, int *addrlen, char *str_addr, char *protocol);

/*
 * This function reports the error and exits back to the shell:
 */
static void displayError(const char *on_what) {
  fputs(strerror(errno), stderr);
  fputs(": ", stderr);
  fputs(on_what, stderr);
  fputc('\n', stderr);
  exit(1);
}

static int sock;                   /* Socket */
static struct sockaddr_in adr_bc;  /* AF_INET */
static struct sockaddr_in adr_ask; /* AF_INET */
static int so_reuseaddr = TRUE;

static int answer(char *dgram, size_t n, const void *buf, size_t len) {
  int z;
  char *ask_addr;
  struct sockaddr_in adr_ask; /* AF_INET */
  int len_bc;
  int len_ask;

  ask_addr = strndup(dgram, n);

  z = mkaddr(&adr_ask, &len_ask, ask_addr, "udp");
  if (z < 0) {
    displayError("Bad ask address");
  }

  len_bc = sizeof(adr_bc);

  /*
   * Broadcast the updated info:
   */
  z = sendto(sock, buf, len, 0, (struct sockaddr *)&adr_ask, len_ask);

  if (z == -1) {
    displayError("sendto()");
  }

  char from[INET_ADDRSTRLEN], to[INET_ADDRSTRLEN];
  printf(">> %s:%d -> %s:%d\n",
         inet_ntop(AF_INET, &adr_bc.sin_addr, from, len_bc),
         ntohs(adr_bc.sin_port),
         inet_ntop(AF_INET, &adr_ask.sin_addr, to, len_ask),
         ntohs(adr_ask.sin_port));

  free(ask_addr);
  return 0;
}

int main(int argc, char **argv) {
  int z;
  socklen_t x;
  int len_bc;      /* length */
  char dgram[512]; /* Recv buffer */
  static char *srv_addr = "*:0xcafe", *bc_addr = "127.255.255.2:0xcafe";

  /*
   * Use a server address from the command line, if one has been provided.
   * Otherwise, this program will default to using the arbitrary address
   * 127.0.0.:
   */
  if (argc > 2) /* Server address: */
    srv_addr = argv[2];

  if (argc > 1) /* Broadcast address: */
    bc_addr = argv[1];

  printf("bc_addr: %s, srv_addr: %s\n", bc_addr, srv_addr);

  /*
   * Create a UDP socket to use:
   */
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock == -1) displayError("socket()");

  /*
   * Form the broadcast address:
   */
  len_bc = sizeof(adr_bc);

  z = mkaddr(&adr_bc, &len_bc, bc_addr, "udp");

  if (z < 0) displayError("Bad broadcast address");

  /*
   * Allow multiple listeners on the broadcast address:
   */
  z = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr,
                 sizeof(so_reuseaddr));

  if (z == -1) displayError("setsockopt(SO_REUSEADDR)");

  /*
   * Bind our socket to the broadcast address:
   */
  z = bind(sock, (struct sockaddr *)&adr_bc, len_bc);

  if (z == -1) displayError("bind(2)");

  char ipbuf[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &adr_bc.sin_addr, ipbuf, len_bc);

  for (;;) {
    /*
     * Wait for a broadcast message:
     */
    x = sizeof(adr_ask);
    z = recvfrom(sock,                        /* Socket */
                 dgram,                       /* Receiving buffer */
                 sizeof(dgram),               /* Max rcv buf size */
                 0,                           /* Flags: no options */
                 (struct sockaddr *)&adr_ask, /* Addr */
                 &x);                         /* Addr len, in & out */

    if (z == -1) displayError("recvfrom(2)"); /* else err */

    char from[INET_ADDRSTRLEN], to[INET_ADDRSTRLEN];
    printf("<< %s:%d -> %s:%d\n",
           inet_ntop(AF_INET, &adr_ask.sin_addr, from, x),
           ntohs(adr_ask.sin_port),
           inet_ntop(AF_INET, &adr_bc.sin_addr, to, len_bc),
           ntohs(adr_bc.sin_port));

    if (answer(dgram, z, srv_addr, strlen(srv_addr)) < 0) {
      // error
    }
  }

  return 0;
}
