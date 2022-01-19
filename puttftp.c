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

#define BUF_SIZE 500

char* buildWRQ(char *file,int WRQ_Size, char *blocksize) {
	char *WRQ;
	WRQ = (char *) malloc(WRQ_Size);
	int blcksize = atoi(blocksize); //To convert the char * into an int
	if (blcksize < 8 || blcksize > 65464) {
		printf("Incompatible blocksize. Must be between 8 and 65464.\n");
		exit(EXIT_FAILURE);
	}
	else {
		int n = strlen(file);
		int l = strlen("octet");
		WRQ[0]=0;
		WRQ[1]=2;
		strcpy(&WRQ[2], file);
		WRQ[2+n]=0;
		strcpy(&WRQ[3+n], "octet");
		WRQ[3+n+l]=0;
		strcpy(&WRQ[4+n+l], "blksize");
		int b = strlen("blksize");
		WRQ[4+n+l+b] = 0;
		strcpy(&WRQ[5+n+l+b],blocksize); //The 0 byte is already in blocksize !
	}
	return WRQ;
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

char* buildDATA(char *buf, int block, int size) {
	char *data;
	data = (char *) malloc(size);
	data[0] = 0;
	data[1] = 3;
	data[2] = 0;
	data[3] = block;
	strcpy(&data[4],buf);
	return data;
}

void puttftp(char *host, char *file, char *blocksize) {

    struct addrinfo hints;
    struct addrinfo *result;
    struct sockaddr srv_addr;
    socklen_t srv_addrlen = sizeof(srv_addr);
    char hbuf[BUF_SIZE];
    char sbuf[BUF_SIZE];


    /* Obtain address matching host/port */
    memset(&hints, 0, sizeof(struct addrinfo)); //Fill memory with constant byte (0)
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_UDP; /* UDP protocol */

    int s = getaddrinfo(host, "1069", &hints, &result); //Put port to 69 for external server, 1069 for local server
	if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
        }
    getnameinfo(result->ai_addr, result->ai_addrlen,hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
    printf("IP : %s, port : %s \n",hbuf,sbuf); //hbuf IP, sbuf port
	
	
	/*Creation of a socket*/
    int sfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sfd==-1) {
		printf("Socket Failure");
		exit(EXIT_FAILURE);
		}
    freeaddrinfo(result); //No longer needed
	
	
	/*Send WRQ with blocksize option*/
	int WRQ_SIZE = 2+strlen(file)+1+strlen("octet")+1+strlen("blcksize")+1+strlen(blocksize);
	char *WRQ;
	WRQ = (char *) malloc(WRQ_SIZE);
	WRQ = buildWRQ(file, WRQ_SIZE, blocksize);
	sendto(sfd, WRQ, WRQ_SIZE, 0, (struct sockaddr *) result->ai_addr, result->ai_addrlen);
	free(WRQ);
	
	
	/*Read OACK*/
	char *OACK;
	int OACK_SIZE = 2 + strlen("blcksize") + strlen(blocksize);
	OACK = (char *) malloc(OACK_SIZE);
	recvfrom(sfd, OACK, OACK_SIZE, 0, &srv_addr, &srv_addrlen);
	printf("New port : %d \n",htons (((struct sockaddr_in *) &srv_addr)->sin_port));
	
	
	/*Get back the accepted blocksize*/
	char *blcksize;
	blcksize = (char *) malloc(strlen(blocksize));
	blcksize = &OACK[2+strlen("blcksize")];
	printf("Accepted blocksize : %d\n",atoi(blcksize));
	
	if (atoi(blcksize) <= atoi(blocksize)) { //Check if accepted blocksize (by the server) is <= to the given one (from client) else send error packet
		
		/*Starting to send and check acknowledgments*/
		char *buf;
		buf = (char *) malloc(atoi(blcksize));
		
		char *ACK;
		ACK = (char *) malloc(4);
		
		int fdr = open(file, 0, O_RDONLY);
		if (fdr == -1) {
			perror("open");
			printf("Error opening file\n");
			exit(EXIT_FAILURE);
		}
		
		int block = 1;
		int cnt = 0; //To count number of times data is sent before receiving ACK or not, arbitrary set at 3
		
		ssize_t nread = read(fdr, buf, atoi(blcksize));
		if (nread == -1) {
			perror("read");
			printf("Error reading file\n");
			exit(EXIT_FAILURE);
		}
		
		while (nread >= 0) { //While there are bytes to read in the file
			char *data;
			if (nread == 0) {
				data = (char *) realloc(data, 0+2+2);
				data = buildDATA(buf, block, 0+2+2);
				sendto(sfd, data, 0+2+2, 0, &srv_addr, srv_addrlen); 
				while (ACK[3] != block && cnt < 3) {
					sendto(sfd, data, 0+2+2, 0, &srv_addr, srv_addrlen); //Resend data
					recvfrom(sfd, ACK, 4, 0, &srv_addr, &srv_addrlen); //Wait for ACK
					cnt++;
				}
				printf("Transmission complete.\n");
				close(fdr);
				free(buf);
				free(ACK);
				free(data);
				exit(EXIT_SUCCESS);
			}
			else {
				data = buildDATA(buf, block, nread+2+2);
				sendto(sfd, data, nread+2+2, 0, &srv_addr, srv_addrlen);
				recvfrom(sfd, ACK, 4, 0, &srv_addr, &srv_addrlen); //Wait for ACK
				while (ACK[3] != block && cnt < 3) {
					sendto(sfd, data, atoi(blcksize)+2+2, 0, &srv_addr, srv_addrlen); //Resend data
					recvfrom(sfd, ACK, 4, 0, &srv_addr, &srv_addrlen); //Wait for ACK
					ACK = realloc(ACK,4);
					cnt++;
				}
				block++;
				buf = (char *) realloc(buf, atoi(blcksize));
				nread = read(fdr, buf, atoi(blcksize));
				if (nread != 0) {
					data = (char *) realloc(data, nread+2+2);
				}
			}
		}
	}
}

int main(int argc, char *argv[]) { //argv0 : input command, argc : number of args, argv1 : first arg (host), argv2 second arg (file), argv3 : third arg (blocksize)	
	if (argc==4) { //Check if all arguments are given
		puttftp(argv[1], argv[2], argv[3]);
	}
	else {
		printf("Error\nUsage : ./putttftp [host] [file] [blocksize]\n");
		exit(EXIT_FAILURE);
	}
	return 0;
}
