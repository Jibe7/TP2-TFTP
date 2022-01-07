#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUF_SIZE 500

void gettftp(char *host, char *file) {

    struct addrinfo hints;
    struct addrinfo *result;
    char hbuf[BUF_SIZE];
    char sbuf[BUF_SIZE];

   /* Obtain address(es) matching host/port */

    memset(&hints, 0, sizeof(struct addrinfo)); // fill memory with constant byte : void *memset(void *s, int c, size_t n);
    hints.ai_family = AF_INET;    /* Allow IPv4 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_UDP;          /* UDP protocol */

    int s = getaddrinfo(host, "1069", &hints, &result);
	if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }
    getnameinfo(result->ai_addr, result->ai_addrlen,hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
      printf("IP : %s\nport : %s \nIPVx : %d\n",hbuf,sbuf,result->ai_family); //hbuf IP, sbuf port

    int sfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    int i = connect(sfd, result->ai_addr, result->ai_addrlen);
	if (i==-1) { printf("Connection error\n"); exit(EXIT_FAILURE); }
	else { printf("Connection Successful !\n"); }
   freeaddrinfo(result);           /* No longer needed */

}

int main(int argc, char *argv[]) {
	//argv0 command entrée, argc nbre d'arguments, argv1 premier argument (host), argv2 deuxième (file)
  if (argc==3) {
	  gettftp(argv[1], argv[2]);
  }
	return 0;
}
