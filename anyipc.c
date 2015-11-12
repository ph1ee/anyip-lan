/* udp-broadcast-server.c:
 * udp broadcast server example
 * Example Stock Index Broadcast:
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

extern int mkaddr(void *addr, int *addrlen, char *str_addr, char *protocol);

#define INTERVAL 4

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

static struct sockaddr_in adr_bc;  /* AF_INET */
static struct sockaddr_in adr_srv; /* AF_INET */
static int so_reuseaddr = TRUE;
static int so_broadcast = TRUE;
static int s; /* Socket */

void *receive_unicast(void *arg) {
  int z;
  socklen_t x;
  socklen_t len_srv;      /* length */
  struct sockaddr_in adr; /* AF_INET */
  char dgram[512];        /* Recv buffer */

  for (;;) {
    /*
     * Wait for a broadcast message:
     */
    x = sizeof(adr);
    z = recvfrom(s,                       /* Socket */
                 dgram,                   /* Receiving buffer */
                 sizeof(dgram),           /* Max rcv buf size */
                 0,                       /* Flags: no options */
                 (struct sockaddr *)&adr, /* Addr */
                 &x);                     /* Addr len, in & out */

    if (z == -1) displayError("recvfrom(2)"); /* else err */

    /* Exclude packet from loopback interface */
    if (adr.sin_addr.s_addr != ntohl(INADDR_LOOPBACK)) {
      len_srv = sizeof(adr_srv);
      char from[INET_ADDRSTRLEN], to[INET_ADDRSTRLEN];
      printf("<< %s:%d -> %s:%d\n", inet_ntop(AF_INET, &adr.sin_addr, from, x),
             ntohs(adr.sin_port),
             inet_ntop(AF_INET, &adr_srv.sin_addr, to, len_srv),
             ntohs(adr_srv.sin_port));

      fwrite(dgram, z, 1, stdout);
      putchar('\n');

      fflush(stdout);
    }
  }

  return NULL;
}

int main(int argc, char **argv) {
  char ep[INET_ADDRSTRLEN + 16]; /* Buffer and ptr */
  int z;                         /* Status return code */
  int len_srv;                   /* length */
  int len_bc;                    /* length */
  static char *srv_addr = "*:0xcafe", *bc_addr = "127.255.255.2:0xcafe";
  pthread_t receive_task;

  printf("bc_addr: %s, srv_addr: %s\n", bc_addr, srv_addr);

  /*
   * Form a server address:
   */
  if (argc > 2) /* Server address: */
    srv_addr = argv[2];

  if (argc > 1) /* Broadcast address: */
    bc_addr = argv[1];

  /*
   * Form the server address:
   */
  len_srv = sizeof(adr_srv);

  z = mkaddr(&adr_srv, /* Returned address */
             &len_srv, /* Returned length */
             srv_addr, /* Input string addr */
             "udp");   /* UDP protocol */

  if (z < 0) displayError("Bad server address");

  /*
   * Form the broadcast address:
   */
  len_bc = sizeof(adr_bc);

  z = mkaddr(&adr_bc, /* Returned address */
             &len_bc, /* Returned length */
             bc_addr, /* Input string addr */
             "udp");  /* UDP protocol */

  if (z < 0) displayError("Bad broadcast address");

  /*
   * Create a UDP socket to use:
   */
  s = socket(AF_INET, SOCK_DGRAM, 0);
  if (s == -1) displayError("socket()");

  /*
   * Allow multiple listeners on the broadcast address:
   */
  z = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr,
                 sizeof(so_reuseaddr));

  if (z == -1) displayError("setsockopt(SO_REUSEADDR)");

  /*
   * Allow broadcasts:
   */
  z = setsockopt(s, SOL_SOCKET, SO_BROADCAST, &so_broadcast,
                 sizeof(so_broadcast));

  if (z == -1) displayError("setsockopt(SO_BROADCAST)");

  /*
   * Bind an address to our socket, so that client programs can listen to this
   * server:
   */
  z = bind(s, (struct sockaddr *)&adr_srv, len_srv);

  if (z == -1) displayError("bind()");

  pthread_create(&receive_task, NULL, receive_unicast, srv_addr);

  for (;;) {
    /*
     * Form a packet to send out:
     */

    char ipbuf[INET_ADDRSTRLEN];
    snprintf(ep, sizeof(ep), "%s:%u",
             inet_ntop(AF_INET, &adr_srv.sin_addr, ipbuf, len_srv),
             ntohs(adr_srv.sin_port));

    /*
     * Broadcast the updated info:
     */
    z = sendto(s, ep, strlen(ep), 0, (struct sockaddr *)&adr_bc, len_bc);

    if (z == -1) displayError("sendto()");

    char from[INET_ADDRSTRLEN], to[INET_ADDRSTRLEN];
    printf(">> %s:%d -> %s:%d [%s]\n",
           inet_ntop(AF_INET, &adr_srv.sin_addr, from, len_srv),
           ntohs(adr_srv.sin_port),
           inet_ntop(AF_INET, &adr_bc.sin_addr, to, len_bc),
           ntohs(adr_bc.sin_port), ep);

    sleep(INTERVAL);
  }

  return 0;
}
