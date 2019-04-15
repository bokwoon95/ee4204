#include "headsock.h"

// Function Protoypes
void readfromsocket(int sockfd);
void send_ack(int sockfd, struct sockaddr *client_addr, socklen_t client_addrlen);

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
    struct sockaddr_in server_addr_in;
    server_addr_in.sin_family = AF_INET;
    server_addr_in.sin_port = htons(MYUDP_PORT); // htons() converts port number to big endian
    server_addr_in.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY=0, 0 means receive data from any IP address
    bzero(&(server_addr_in.sin_zero), 8);
    // Typecast internet socket address to generic socket address
    struct sockaddr *server_addr = (struct sockaddr *)&server_addr_in;
    socklen_t server_addrlen = sizeof(struct sockaddr);

    // Create UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { printf("error creating socket"); exit(1); }

    // Bind socket address to socket (server only, client don't need)
    if (bind(sockfd, server_addr, server_addrlen) == -1) {
        printf("error in binding");
        exit(1);
    }

    // Read file data from socket
    while (1) {
        printf("Ready to receive data\n");
        readfromsocket(sockfd);
    }
    close(sockfd);
}

void readfromsocket(int sockfd) {
    char packet[4 * DATAUNIT]; // buffer used to contain the incoming packet. Max size is 4 * DATAUNIT

    // Create empty struct to contain client address
    // It will be filled in by recvfrom()
    struct sockaddr client_addr;
    socklen_t client_addrlen = sizeof(struct sockaddr);

    // Get filesize from client and allocate filebuffer space accordingly
    long filesize;
    int n = recvfrom(sockfd, &filesize, sizeof(filesize), 0, &client_addr, &client_addrlen);
    if (n < 0) { printf("error in receiving packet\n"); exit(1); }
    send_ack(sockfd, &client_addr, client_addrlen); // Acknowledge that filesize has been received
    printf("The file is %ld bytes big\n", filesize);
    char filebuffer[filesize];

    long fileoffset = 0; // Tracks how many bytes have been received so far
    char packetlastbyte; // Tracks the last byte of the latest packet received
    // Keep reading from socket until the last byte of the latest packet is an End of Transmission character 0x4
    do {
        // Read incoming packet (recvfrom will block until data is received)
        int bytesreceived = recvfrom(sockfd, &packet, 4*DATAUNIT, 0, &client_addr, &client_addrlen);
        if (bytesreceived < 0) { printf("error in receiving packet\n"); exit(1); }

        // Append packet data to filebuffer
        memcpy((filebuffer + fileoffset), packet, bytesreceived);
        fileoffset += bytesreceived;
        packetlastbyte = packet[bytesreceived-1]; // last byte's index is bytesreceived-1 because index starts from 0

        // Acknowledge that packet has been received
        send_ack(sockfd, &client_addr, client_addrlen);
    } while (packetlastbyte != 0x4);
    fileoffset-=1; // Disregard the last byte of filebuffer because it is the End of Transmission character 0x4

    // Open file for writing
    char filename[] = "myUDPreceive.txt";
    FILE* fp = fopen(filename, "wt");
    if (fp == NULL) { printf("File %s doesn't exist\n", filename); exit(1); }

    /* Copy the filebuffer contents into file
    fwrite(void *ptr,        Buffer address
           size_t size,      Size of each element to be copied
           size_t count,     Number of elements to be copied
           FILE *stream      File input stream
           ); */
    fwrite(filebuffer, 1, fileoffset, fp);
    fclose(fp);
    printf("A file has been received\n total data received is %d bytes\n\n", (int)fileoffset);
}

void send_ack(int sockfd, struct sockaddr *addr, socklen_t addrlen) {
    const int ACKNOWLEDGE = 1;
    int ack_sent = 0;
    int ack_thresh = 10;
    while (!ack_sent) {
        if (sendto(sockfd, &ACKNOWLEDGE, sizeof(ACKNOWLEDGE), 0, addr, addrlen) >= 0) {
            ack_sent = 1;
            printf("ACKNOWLEDGE sent\n");
        } else {
            if (ack_thresh-- <= 0) {
                printf("emergency breakout of ack error loop\n");
                exit(1);
            } else printf("error sending ack, trying again..\n");
        }
    }
}
