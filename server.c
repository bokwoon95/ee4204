#include "headsock.h"

// Function Protoypes
void readfromsocket(int sockfd);
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
    while (1) {
        readfromsocket2(sockfd);
    }

    // Close socket
    close(sockfd);
    exit(0);
}

void readfromsocket2(int sockfd) {
    char filebuffer[BUFSIZE];
    char recvbuffer[4*PACKLEN];

    // A & B are optional arguments for recvfrom/sendto, we don't care what their value is as long it's a sockaddr struct
    struct sockaddr A;
    struct sockaddr *Ap = &A;
    socklen_t B = sizeof(struct sockaddr);
    socklen_t *Bp = &B;

    struct ack_so ack;
    ack.num = 1;
    ack.len = 0;

    long filesize = 0;
    int n = 0;
    FILE *fp;
    printf("Ready to receive data\n");
    int acked = 1;
    char packetlastbyte;
    // Keep reading until an End of Transmission character 0x4 is received
    do {
        if (acked) {
            if ((n = recvfrom(sockfd, &recvbuffer, 2*PACKLEN, 0, Ap, Bp)) == -1) {
                printf("file receive error\n");
                exit(1);
            }
            memcpy((filebuffer + filesize), recvbuffer, n); // copies memory area, copies n byte from recvbuffer and place it into area dest as buffer + filesize
            filesize += n; //next memcopy area
            acked = 0; //wait ack
            packetlastbyte = recvbuffer[n-1];
        }
        if (!acked) {
            if ((n = sendto(sockfd, &ack, 2, 0, Ap, B)) == -1) {
                printf("error sending ack\n");
            } else {
                if (ack.num == 1 && ack.len == 0) {
                    acked = 1; //succesful acknowledge
                }
            }
        }
    } while (packetlastbyte != 0x4);
    filesize-=1; // Disregard the End of Transmission character 0x4

    // Write buffer to file
    if ((fp = fopen("myUDPreceive.txt", "wt")) == NULL) {
        printf("file write error\n");
        exit(0);
    }
    fwrite(filebuffer, 1, filesize, fp);
    fclose(fp);
    printf("A file has been received\n total data received is %d bytes\n\n", (int)filesize);
}

void readfromsocket(int sockfd) {
}
