#include "headsock.h"

// Function Protoypes
void readfromsocket(int sockfd);

int main(int argc, char *argv[]) {
    /* Create internet socket addr
       struct fields must be in network-byte order (big endian)
       struct sockaddr_in {
           short sin_family;          boilerplate=AF_INET
           ushort sin_port;           TCP/UDP port
           struct sin_addr {
               ulong s_addr;          IPv4 address(es) you wish to receive from
           }
           unsigned char[8] sin_zero; boilerplate=structure padding(initialize to all 0s)
       } */
    struct sockaddr_in addr_in;
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(MYUDP_PORT); // htons() converts port number to big endian
    addr_in.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY=0, 0 means receive data from any IP address
    bzero(&(addr_in.sin_zero), 8);
    // Typecast internet socket address to generic socket address
    struct sockaddr *addr = (struct sockaddr *)&addr_in;
    socklen_t addrlen = sizeof(struct sockaddr);

    // Create UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { printf("error in socket"); exit(1); }

    // Bind socket config to socket (server only, client don't need)
    if (bind(sockfd, addr, addrlen) == -1) {
        printf("error in binding");
        exit(1);
    }

    // Read file data from socket
    while (1) { readfromsocket(sockfd); }
    close(sockfd);
}

void readfromsocket(int sockfd) {
    char filebuffer[BUFSIZE]; // buffer used to contain the entire file
    char packet[4*PACKLEN]; // buffer used to contain the incoming packet

    // create empty struct for the client address (for recvfrom() to use)
    struct sockaddr clientaddr;
    socklen_t clientaddrlen = sizeof(struct sockaddr);

    struct ack_so ack;
    ack.num = 1;
    ack.len = 0;

    long fileoffset = 0; // Tracks how many bytes have been received so far
    char packetlastbyte; // Tracks the last byte of the latest packet received
    // Keep reading until a packet ends with a End of Transmission character 0x4
    printf("Ready to receive data\n");
    do {
        // Read incoming packet (recvfrom will block until data is received)
        int bytesreceived = recvfrom(sockfd, &packet, 4*PACKLEN, 0, &clientaddr, &clientaddrlen);
        if (bytesreceived < 0) printf("error in receiving packet\n");
        // Append packet data to filebuffer
        memcpy((filebuffer + fileoffset), packet, bytesreceived);
        fileoffset += bytesreceived;
        packetlastbyte = packet[bytesreceived-1];

        // Send Acknowledge
        int ack_sent = 0;
        int ack_thresh = 10;
        while (!ack_sent) {
            if (sendto(sockfd, &ack, 2, 0, &clientaddr, clientaddrlen) < 0) {
                if (ack_thresh-- <= 0) {
                    printf("emergency breakout of ack error loop\n");
                    exit(1);
                }
                printf("error sending ack, trying again..\n");
            } else {
                if (ack.num == 1 && ack.len == 0) {
                    ack_sent = 1;
                }
            }
        }
    } while (packetlastbyte != 0x4);
    fileoffset-=1; // Disregard the End of Transmission character 0x4

    // Open file for writing
    char filename[] = "myUDPreceive.txt";
    FILE* fp = fopen(filename, "wt");
    if (fp == NULL) { printf("File %s doesn't exist\n", filename); exit(1); }

    /* Copy the filebuffer contents into file
    fwrite(void *ptr,        Buffer address
           size_t size,      Size of each element
           size_t count,     Number of elements to be copied
           FILE *stream      File input stream
           );
    */
    fwrite(filebuffer, 1, fileoffset, fp);
    fclose(fp);
    printf("A file has been received\n total data received is %d bytes\n\n", (int)fileoffset);
}
