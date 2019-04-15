#include "headsock.h"
#include <arpa/inet.h>

// Function Prototypes
void sendtosocket(int sockfd, struct sockaddr *addr, socklen_t addrlen);
void tv_sub(struct  timeval *out, struct timeval *in); //calculate the time interval between out and in
void wait_ack(int sockfd, struct sockaddr *addr, socklen_t addrlen);

int main(int argc, char *argv[]) {
    if (argc < 1) {
        printf("   Usage: ./server.out <dataunits>, dataunits is in bytes\n");
        exit(1);
    }
    char *IP_addr = argv[1];

    /* Create internet socket address
       struct fields must be in network-byte order (big endian)
       struct sockaddr_in {
           short sin_family;          boilerplate=AF_INET
           ushort sin_port;           TCP/UDP port
           struct sin_addr {
               ulong s_addr;          IPv4 address(es) you wish to send to
           }
           unsigned char[8] sin_zero; boilerplate=structure padding(initialize to all 0s)
       } */
    struct sockaddr_in server_addr_in;
    server_addr_in.sin_family = AF_INET;
    server_addr_in.sin_port = htons(MYUDP_PORT); // htons() converts port number to big endian form
    server_addr_in.sin_addr.s_addr = inet_addr(IP_addr); // inet_addr converts IP_addr string to big endian form
    bzero(&(server_addr_in.sin_zero), 8);
    // Typecast internet socket address to generic socket address
    struct sockaddr *server_addr = (struct sockaddr *)&server_addr_in;
    socklen_t server_addrlen = sizeof(struct sockaddr);

    // Create UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { printf("error creating socket"); exit(1); }

    // Send file data to socket
    sendtosocket(sockfd, server_addr, server_addrlen);
    close(sockfd);
}

void sendtosocket(int sockfd, struct sockaddr *server_addr, socklen_t server_addrlen) {
    char packet[DATAUNIT]; // buffer used to contain the incoming packet

    // Open file for reading
    char filename[] = "myfile.txt";
    FILE* fp = fopen(filename, "rt");
    if (fp == NULL) { printf("File %s doesn't exist\n", filename); exit(1); }

    // Get the total filesize and allocate filebuffer space accordingly
    fseek(fp, 0, SEEK_END); long filesize = ftell(fp); rewind(fp);
    printf("The file length is %d bytes\n", (int)filesize);
    char filebuffer[filesize]; // buffer used to contain the entire file
    /* Copy the file contents into filebuffer
    fread(void *ptr,        Buffer address
          size_t size,      Size of each element to be copied
          size_t count,     Number of elements to be copied
          FILE *stream      File input stream
          ); */
    fread(filebuffer, 1, filesize, fp);
    filebuffer[filesize]=0x4; // Append the filebuffer contents with the End of Transmission ASCII character
    filesize+=1; // Increase filesize by 1 byte because of the addition of the EOT ASCII character
    fclose(fp);

    // Tell server how big the file is
    sendto(sockfd, &filesize, sizeof(filesize), 0, server_addr, server_addrlen);
    wait_ack(sockfd, server_addr, server_addrlen);

    // Get start time
    struct timeval timeSend;
    gettimeofday(&timeSend, NULL);

    long fileoffset = 0; // Tracks how many bytes have been sent so far
    int dum = 1; // data unit multiple
    while (fileoffset < filesize) {
        for (int i=0; i<dum; i++) {
            int packetsize = (DATAUNIT < filesize - fileoffset) ? DATAUNIT : filesize - fileoffset;
            // Copy the next section of the filebuffer into the packet
            memcpy(packet, (filebuffer + fileoffset), packetsize);
            // Update fileoffset
            fileoffset += packetsize;
            // Send packet data into socket
            int n = sendto(sockfd, &packet, packetsize, 0, server_addr, server_addrlen);
            if (n < 0) printf("error in sending packet\n");
            printf("packet of size %d sent\n", packetsize);
        }
        wait_ack(sockfd, server_addr, server_addrlen);
        dum = (++dum % 5 == 0) ? 1 : dum % 5; // dum alternates between 1,2,3,4
    }

    // Get end time
    struct timeval timeRcv;
    gettimeofday(&timeRcv, NULL);

    // Calculate and print transfer rate
    tv_sub(&timeRcv, &timeSend);
    float time = (timeRcv.tv_sec)*1000.0 + (timeRcv.tv_usec)/1000.0;
    printf("Time(ms) : %.3f, Data sent(byte): %d\nData rate: %.3f (Kbytes/s)\n", time, (int)fileoffset, fileoffset/time/1000);
}

void tv_sub(struct  timeval *out, struct timeval *in) {
    if ((out->tv_usec -= in->tv_usec) <0) {
        --out ->tv_sec;
        out ->tv_usec += 1000000;
    }
    out->tv_sec -= in->tv_sec;
}

void wait_ack(int sockfd, struct sockaddr *addr, socklen_t addrlen) {
    int ack_received = 0;
    int ACKNOWLEDGE = 0;
    while (!ack_received) {
        if (recvfrom(sockfd, &ACKNOWLEDGE, sizeof(ACKNOWLEDGE), 0, addr, &addrlen) >= 0) {
            if (ACKNOWLEDGE == 1) {
                ack_received = 1;
                printf("ACKNOWLEDGE received\n");
            } else {
                printf("ACKNOWLEDGE received but value was not 1\n");
                exit(1);
            }
        } else {
            printf("error when waiting for acknowledge\n");
            exit(1);
        }
    }
}
