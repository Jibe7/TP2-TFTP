#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <fcntl.h>

#define BUF_SIZE 1024

char* buildRRQ(char *file,int RRQ_Size, int blocksize) { ////Verify blocksize !
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
	if (blocksize != 0) { //If the user wants to add a blocksize option
		strcpy(&RRQ[4+n+l], "blksize");
		int b = strlen("blksize");
		RRQ[4+n+l+b] = 0;
		RRQ[5+n+l+b] = blocksize; //Check between 8 and 1428 bytes
		//RRQ[5+n+l+b+blocksize/8] = 0;
	}
	return RRQ;
}

void gettftp(char *host, char *file, int blocksize) {

    struct addrinfo hints;
    struct addrinfo *result;
    struct sockaddr srv_addr;
    socklen_t srv_addrlen = sizeof(srv_addr);
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
		exit(EXIT_FAILURE);
	}
    freeaddrinfo(result);           /* No longer needed */
	
	
	int RRQ_SIZE = 2+strlen(file)+1+strlen("octet")+1;
	char *RRQ;
	RRQ = (char *) malloc(RRQ_SIZE);
	RRQ = buildRRQ(file, RRQ_SIZE, blocksize);  
	
	sendto(sfd, RRQ, RRQ_SIZE, 0, (struct sockaddr *) result->ai_addr, result->ai_addrlen);
	free(RRQ);
	
	char *buf;
	buf = (char *) malloc(BUF_SIZE);
	
	ssize_t nread = recvfrom(sfd, buf, BUF_SIZE, 0, &srv_addr, &srv_addrlen);
	printf("New port : %d \n",htons (((struct sockaddr_in *) &srv_addr)->sin_port));
	
	char *ACK;
	ACK = (char *) malloc(4);
	
	if (nread == -1) {
		perror("read");
		printf("Error");
		exit(EXIT_FAILURE);
	}

	int fdc = open("test.txt", O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP);
	int block = 0;
	
	while (buf[3] != block) {
		write(fdc,&buf[4],nread-4);
		ACK[0] = 0;
		ACK[1] = 4;
		ACK[2] = buf[2];
		ACK[3] = buf[3];
		sendto(sfd, ACK, 4, 0, &srv_addr, srv_addrlen);
		buf = (char *) realloc(buf, BUF_SIZE);
		nread = recvfrom(sfd, buf, BUF_SIZE, 0, &srv_addr, &srv_addrlen);
		if (nread != 4) {
			block++;
		}
		else {
			block += 2;
		}
	}
	ACK[0] = 0;
	ACK[1] = 4;
	ACK[2] = buf[2];
	ACK[3] = buf[3];
	sendto(sfd, ACK, 4, 0, &srv_addr, srv_addrlen);
	
	buf = (char *) realloc(buf, BUF_SIZE);
	close(fdc);
	
}

int main(int argc, char *argv[]) {
	//argv0 command entrée, argc nbre d'arguments, argv1 premier argument (host), argv2 deuxième (file), argv3 = blocksize
  if (argc==4) {
	  gettftp(argv[1], argv[2], (int) argv[3]); //Casting not really liked
  }
	return 0;
}



