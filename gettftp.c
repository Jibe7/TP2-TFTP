#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUF_SIZE 500

char* buildRRQ(char *file,int RRQ_Size) {
	int n = strlen(file);
	char *octet = "octet";
	int l = strlen(octet);
	char *RRQ;
	RRQ = (char *) malloc(RRQ_Size);
	for (int i=0;i<15;i++) {
		RRQ[i]=0;
	}
	RRQ[15]=1;
	strcpy(&RRQ[16], file);
	for (int j=0;j<l;j++) {
		RRQ[16+n+j]=0;
	}
	strcpy(&RRQ[24+n], octet);
	for (int j=0;j<8;j++) {
		RRQ[24+n+l+j]=0;
	}
	return RRQ;
}

void gettftp(char *host, char *file) {

    struct addrinfo hints;
    struct addrinfo *result;
    char hbuf[BUF_SIZE];
    char sbuf[BUF_SIZE];

   /* Obtain address(es) matching host/port */

    memset(&hints, 0, sizeof(struct addrinfo)); // fill memory with constant byte : void *memset(void *s, int c, size_t n);
    //hints.ai_family = AF_INET;    /* Allow IPv4 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_UDP;          /* UDP protocol */

    int s = getaddrinfo(host, "1069", &hints, &result);
	if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }
    getnameinfo(result->ai_addr, result->ai_addrlen,hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
      printf("IP : %s\nport : %s \n",hbuf,sbuf); //hbuf IP, sbuf port

    int sfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sfd==-1) {
		printf("Socket Failure");
	}
    freeaddrinfo(result);           /* No longer needed */
	
	
	int RRQ_SIZE = 16+strlen(file)+8+strlen("netascii")+8;
	char *RRQ;
	RRQ = (char *) malloc(RRQ_SIZE);
	
	RRQ = buildRRQ(file, RRQ_SIZE);  
	for (int i = 0; i < RRQ_SIZE; i++) {
		printf("%d",RRQ[i]);
	}
	printf("\n");
	sendto(sfd, RRQ, RRQ_SIZE, 0, (struct sockaddr *) result->ai_addr, result->ai_addrlen);
}

int main(int argc, char *argv[]) {
	//argv0 command entrée, argc nbre d'arguments, argv1 premier argument (host), argv2 deuxième (file)
  if (argc==3) {
	  gettftp(argv[1], argv[2]);
  }
	return 0;
}

