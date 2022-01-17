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

char* buildRRQ(char *file,int RRQ_Size, char *blocksize) {
	char *RRQ;
	RRQ = (char *) malloc(RRQ_Size);
	int blcksize = atoi(blocksize); //To convert the char * into an int
	if (blcksize < 8 || blcksize > 65464) {
		printf("Incompatible blocksize. Must be between 8 and 65464.\n");
		exit(EXIT_FAILURE);
	}
	else {
		int n = strlen(file);
		int l = strlen("octet");
		RRQ[0]=0;
		RRQ[1]=1;
		strcpy(&RRQ[2], file);
		RRQ[2+n]=0;
		strcpy(&RRQ[3+n], "octet");
		RRQ[3+n+l]=0;
		strcpy(&RRQ[4+n+l], "blksize");
		int b = strlen("blksize");
		RRQ[4+n+l+b] = 0;
		strcpy(&RRQ[5+n+l+b],blocksize); //The 0 byte is already in blocksize !
	}
	return RRQ;
}

char* buildACK(char ack2, char ack3) {
	char *ACK;
	ACK = (char *) malloc(4);
	ACK[0] = 0;
	ACK[1] = 4;
	ACK[2] = ack2;
	ACK[3] = ack3;
	return ACK;
}

void gettftp(char *host, char *file, char *blocksize) {

    struct addrinfo hints;
    struct addrinfo *result;
    struct sockaddr srv_addr;
    socklen_t srv_addrlen = sizeof(srv_addr);
    char hbuf[BUF_SIZE];
    char sbuf[BUF_SIZE];

    /* Obtain address matching host/port */
    memset(&hints, 0, sizeof(struct addrinfo)); // fill memory with constant byte : void *memset(void *s, int c, size_t n);
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
    freeaddrinfo(result); //No longer needed
	
	/*Send RRQ with blocksize option*/
	int RRQ_SIZE = 2+strlen(file)+1+strlen("octet")+1+strlen("blcksize")+1+strlen(blocksize);
	char *RRQ;
	RRQ = (char *) malloc(RRQ_SIZE);
	RRQ = buildRRQ(file, RRQ_SIZE, blocksize);
	sendto(sfd, RRQ, RRQ_SIZE, 0, (struct sockaddr *) result->ai_addr, result->ai_addrlen);
	RRQ = (char *) realloc(RRQ, RRQ_SIZE);
	
	/*Read OACK*/
	char *OACK;
	int OACK_SIZE = 2 + strlen("blcksize") + strlen(blocksize);
	OACK = (char *) malloc(OACK_SIZE);
	ssize_t nread = recvfrom(sfd, OACK, OACK_SIZE, 0, &srv_addr, &srv_addrlen);
	char *blcksize;
	blcksize = (char *) malloc(strlen(blocksize));
	blcksize = &OACK[2+strlen("blcksize")];
	printf("Accepted blocksize : %d\n",atoi(blcksize));
	
	//Send ACK with block 0  to approve blocksize and start data transmission
	sendto(sfd, buildACK(0, 0), 4, 0, &srv_addr, srv_addrlen);
	
	char *buf;
	buf = (char *) malloc(BUF_SIZE);
	
	nread = recvfrom(sfd, buf, BUF_SIZE, 0, &srv_addr, &srv_addrlen);
	printf("New port : %d \n",htons (((struct sockaddr_in *) &srv_addr)->sin_port));
	
	
	if (nread == -1) {
		perror("read");
		printf("Error");
		exit(EXIT_FAILURE);
	}

	int fdc = open("test.txt", O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP);
	int block = 0;
	
	while (buf[3] != block) {
		write(fdc,&buf[4],nread-4);
		sendto(sfd, buildACK(buf[2], buf[3]), 4, 0, &srv_addr, srv_addrlen);
		buf = (char *) realloc(buf, BUF_SIZE);
		nread = recvfrom(sfd, buf, BUF_SIZE, 0, &srv_addr, &srv_addrlen);
		if (nread >= atoi(blcksize)+4) { //If it is not the final block
			block++;
		}
		else {
			block += 2; //To get out of the while loop
		}
	}
	sendto(sfd, buildACK(buf[2],buf[3]), 4, 0, &srv_addr, srv_addrlen);
	
	free(OACK);
	free(buf);
	close(fdc);
	
}

int main(int argc, char *argv[]) { //argv0 : input command, argc : number of args, argv1 : first arg (host), argv2 second arg (file), argv3 : third arg (blocksize)	
	if (argc==4) { //Check if all arguments are given
		gettftp(argv[1], argv[2], argv[3]);
	}
	else {
		printf("Error\nUsage : ./getttftp [host] [file] [blocksize]\n");
		exit(EXIT_FAILURE);
	}
	return 0;
}
