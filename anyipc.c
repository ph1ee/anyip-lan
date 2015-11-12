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

static const char *port = "0xcafe", *data = "hello";
static int so_reuseaddr = TRUE;
static int so_broadcast = TRUE;

void *receive_unicast(void *arg) {
  socklen_t x;
  struct sockaddr_in adr; /* AF_INET */
  char dgram[512];        /* Recv buffer */
  int s = (int)arg;
  int z;

  for (;;) {
    /*
     * Wait for a broadcast message:
     */
    memset(dgram, 0, sizeof(dgram));
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
      char from[INET_ADDRSTRLEN];
      printf("<< %s:%d [%s]\n", inet_ntop(AF_INET, &adr.sin_addr, from, x),
             ntohs(adr.sin_port), z == sizeof(dgram) ? "..." : dgram);
    }
  }

  return NULL;
}

int main(int argc, char **argv) {
  int sock;                  /* Socket */
  struct sockaddr_in adr_bc; /* AF_INET */
  int z;                     /* Status return code */

  if (argc > 1) /* Broadcast port: */
    port = argv[1];

  printf("port: %s, data: %s\n", port, data);

  /*
   * Form the broadcast address:
   */

  adr_bc.sin_family = AF_INET;
  adr_bc.sin_addr.s_addr = ntohl(INADDR_BROADCAST);
  adr_bc.sin_port = htons(strtol(port, NULL, 0));

  /*
   * Create a UDP socket to use:
   */
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock == -1) displayError("socket()");

  /*
   * Allow multiple listeners on the broadcast address:
   */
  z = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr,
                 sizeof(so_reuseaddr));

  if (z == -1) displayError("setsockopt(SO_REUSEADDR)");

  /*
   * Allow broadcasts:
   */
  z = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &so_broadcast,
                 sizeof(so_broadcast));

  if (z == -1) displayError("setsockopt(SO_BROADCAST)");

  pthread_t receive_task;
  pthread_create(&receive_task, NULL, receive_unicast, (void *)sock);

  for (;;) {
    /*
     * Form a packet to send out:
     */
    char buf[256];
    snprintf(buf, sizeof(buf), "%s", data);

    /*
     * Broadcast the updated info:
     */
    socklen_t addrlen = sizeof(adr_bc);
    z = sendto(sock, buf, strlen(buf), 0, (struct sockaddr *)&adr_bc, addrlen);

    if (z == -1) displayError("sendto()");

    char to[INET_ADDRSTRLEN];
    printf(">> %s:%d [%s]\n", inet_ntop(AF_INET, &adr_bc.sin_addr, to, addrlen),
           ntohs(adr_bc.sin_port), buf);

    sleep(INTERVAL);
  }

  return 0;
}
