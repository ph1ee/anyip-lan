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

static char *srv_port = "0xcafe";
static int so_reuseaddr = TRUE;

int main(int argc, char **argv) {
  int sock;                  /* Socket */
  struct sockaddr_in adr_bc; /* AF_INET */
  char dgram[512];           /* Recv buffer */
  int z;

  if (argc > 1) /* Broadcast port: */
    srv_port = argv[1];

  printf("srv_port: %s\n", srv_port);

  /*
   * Create a UDP socket to use:
   */
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock == -1) displayError("socket()");

  /*
   * Form the broadcast address:
   */
  adr_bc.sin_family = AF_INET;
  adr_bc.sin_addr.s_addr = htonl(INADDR_ANY);
  adr_bc.sin_port = htons(strtol(srv_port, NULL, 0));

  /*
   * Allow multiple listeners on the broadcast address:
   */
  z = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr,
                 sizeof(so_reuseaddr));

  if (z == -1) displayError("setsockopt(SO_REUSEADDR)");

  /*
   * Bind our socket to the broadcast address:
   */
  socklen_t len_bc = sizeof(adr_bc);
  z = bind(sock, (struct sockaddr *)&adr_bc, len_bc);

  if (z == -1) displayError("bind(2)");

  for (;;) {
    struct sockaddr_in adr_ask; /* AF_INET */

    /*
     * Wait for a broadcast message:
     */
    socklen_t x = sizeof(adr_ask);
    z = recvfrom(sock,                        /* Socket */
                 dgram,                       /* Receiving buffer */
                 sizeof(dgram),               /* Max rcv buf size */
                 0,                           /* Flags: no options */
                 (struct sockaddr *)&adr_ask, /* Addr */
                 &x);                         /* Addr len, in & out */

    if (z == -1) displayError("recvfrom(2)"); /* else err */

    /*
     * Broadcast
     */
    z = sendto(sock, dgram, z, 0, (struct sockaddr *)&adr_ask, x);

    if (z == -1) displayError("sendto()");

    char from[INET_ADDRSTRLEN];
    printf(">> %s:%d [%s]\n", inet_ntop(AF_INET, &adr_ask.sin_addr, from, x),
           ntohs(adr_ask.sin_port), z == sizeof(dgram) ? "..." : dgram);
  }

  return 0;
}
