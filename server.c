#include "headsock.h"

/* void readfromsocket(int sockfd); */
void readfromsocket2(int sockfd);

int main(int argc, char *argv[]) {
    // Create socket
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        printf("error in socket");
        exit(1);
    }

    /* Create socket config struct
       struct fields must be sent in network-byte order (big endian)
       struct sockaddr_in {
       short sin_family;          boilerplate=AF_INET
       ushort sin_port;           TCP/UDP port
       struct sin_addr {
       ulong s_addr;          IPv4 address(es) you wish to receive from
       }
       unsigned char[8] sin_zero; boilerplate=structure padding(initialize to all 0s)
       } */
    struct sockaddr_in my_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(MYUDP_PORT);
    my_addr.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY=0
    bzero(&(my_addr.sin_zero), 8);

    // Bind socket config struct to socket
    if (bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)) == -1) {
        printf("error in binding");
        exit(1);
    }

    // Read file data from socket
    while(1) {
        readfromsocket2(sockfd);
    }

    // Close socket
    close(sockfd);
    exit(0);
}

void readfromsocket2(int sockfd) {
    char buffer[BUFSIZE];  // overall buffer
    char recvs[4*DATALEN]; // intermediate buffer

    /*
    struct ack_so {
        uint8_t num;
        uint8_t len;
    };
    */
    struct ack_so ack;

    long filesize = 0;
    int n = 0;
    int end = 0;
    struct sockaddr_in addr;
    socklen_t len = sizeof(struct sockaddr_in);
    FILE *fp;
    printf("Ready to receive data\n");
    int acked = 1;
    ack.num = 1;
    ack.len = 0;
    while(!end) {
        if (acked) {
            if ((n = recvfrom(sockfd, &recvs, 2*DATALEN, 0, (struct sockaddr *)&addr, &len)) == -1) {
                printf("file receive error\n");
                exit(1);
            }
            if (recvs[n-1] == '\0') {
                end = 1; //end flag, break out of while loop here
                n--; //last char to be ignored \0
            }
            memcpy((buffer + filesize), recvs, n); // copies memory area, copies n byte from recvs and place it into area dest as buffer + filesize
            filesize += n; //next memcopy area
            acked = 0; //wait ack
        }
        if (!acked) {
            if ((n = sendto(sockfd, &ack, 2, 0,(struct sockaddr *)&addr, len)) == -1) {
                printf("error sending ack\n");
            } else {
                if (ack.num == 1 && ack.len == 0) {
                    acked = 1; //succesful acknowledge
                }
            }
        }
    }

    // Write buffer to file
    if ((fp = fopen("myUDPreceive.txt", "wt")) == NULL) { //open file
        printf("file write error\n");
        exit(0);
    }
    fwrite(buffer, 1, filesize, fp);
    fclose(fp);
    printf("A file has been received\n total data received is %d bytes\n", (int)filesize);
}

/* void readfromsocket(int sockfd) { */
/* 	char bigbuffer[BUFSIZE]; */
/* 	char recvs[2*DATALEN]; */
/* 	struct sockaddr_in addr; */
/*  */
/* 	printf("Ready to receive data\n"); */
/* 	int acked = 1; */
/* 	struct ack_so ack; */
/* 	ack.num = 1; */
/* 	ack.len = 0; */
/*  */
/*  */
/* 	#<{(| */
/* 	int recvfrom( */
/* 		int socket, */
/* 		void *buf, */
/* 		size_t buflen, */
/* 		int flags, */
/* 		struct sockaddr *sender_addr,   boilerplate */
/* 		socklen_t *sender_addrlen       boilerplate */
/* 		); */
/* 		returns length of data read. If 0, connection is closed. If -1, error occurred. recvfrom is BLOCKING by default. */
/* 	|)}># */
/* 	int len = sizeof (struct sockaddr_in); */
/* 	int n = 0; */
/* 	if ((n=recvfrom(sockfd, &recvs, MAXSIZE, 0, (struct sockaddr *)&addr, &len)) == -1) { */
/* 		printf("error receiving"); */
/* 		exit(1); */
/* 	} */
/* 	while (!end) { */
/* 		if (acked) { */
/* 			if ((n = recvfrom(sockfd, &recvs, 2*DATALEN, 0, (struct sockaddr *)&addr, &len)) == -1) { */
/* 				printf("file receive error\n"); */
/* 				exit(1); */
/* 			} */
/* 			if (recvs[n-1] == '\0') { */
/* 				end = 1; //end flag, break out of while loop here */
/* 				n--; //last char to be ignored \0 */
/* 			} */
/* 			memcpy((buffer + filesize), recvs, n); // copies memory area, copies n byte from recvs and place it into area dest as buffer + filesize */
/* 			filesize += n; //next memcopy area */
/* 			acked = 0; //wait ack */
/* 		} */
/* 		if (!acked) { */
/* 			if ((n = sendto(sockfd, &ack, 2, 0,(struct sockaddr *)&addr, len)) == -1) { */
/* 				printf("error sending file\n"); */
/* 			} */
/* 			else { */
/* 				if (ack.num == 1 && ack.len == 0) { */
/* 					acked = 1; //succesful acknowledge */
/* 				} */
/* 			} */
/* 		} */
/* 	} */
/* 	recvs[n] = '\0'; */
/* 	printf("the received string is :\n%s", recvs); */
/* } */
