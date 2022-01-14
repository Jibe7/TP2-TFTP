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
	RRQ[0]=0;
	RRQ[1]=1;
	strcpy(&RRQ[2], file);
	RRQ[2+n]=0;
	strcpy(&RRQ[3+n], octet);
	RRQ[3+n+l]=0;
	return RRQ;
}

void gettftp(char *host, char *file) {

    struct addrinfo hints;
    struct addrinfo *result;
    char hbuf[BUF_SIZE];
    char sbuf[BUF_SIZE];
    char buf[BUF_SIZE];

   /* Obtain address(es) matching host/port */

    memset(&hints, 0, sizeof(struct addrinfo)); // fill memory with constant byte : void *memset(void *s, int c, size_t n);
    //hints.ai_family = AF_INET;    /* Allow IPv4 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_UDP;          /* UDP protocol */

    int s = getaddrinfo(host, "69", &hints, &result);
	if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }
    getnameinfo(result->ai_addr, result->ai_addrlen,hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
      printf("IP : %s\nport : %s \n",hbuf,sbuf); //hbuf IP, sbuf port

    int sfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sfd==-1) {
		printf("Socket Failure");
		exit(EXIT_FAILURE);
	}
    freeaddrinfo(result);           /* No longer needed */
	
	
	int RRQ_SIZE = 2+strlen(file)+1+strlen("octet")+1;
	char *RRQ;
	RRQ = (char *) malloc(RRQ_SIZE);
	
	RRQ = buildRRQ(file, RRQ_SIZE);  
	for (int i = 0; i < RRQ_SIZE; i++) {
		printf("%d",RRQ[i]);
	}
	printf("\n");
	
	sendto(sfd, RRQ, RRQ_SIZE, 0, (struct sockaddr *) result->ai_addr, result->ai_addrlen);
	
	int nread = read(sfd,buf,BUF_SIZE);
	if (nread == -1) {
		perror("read");
		exit(EXIT_FAILURE);
	}
	printf("Received %ld bytes : %s\n", (long) nread, buf);
}

int main(int argc, char *argv[]) {
	//argv0 command entrée, argc nbre d'arguments, argv1 premier argument (host), argv2 deuxième (file)
  if (argc==3) {
	  gettftp(argv[1], argv[2]);
  }
	return 0;
}


