#include "headsock.h"
#include <arpa/inet.h>

// Function Prototypes
void sendtosocket(int sockfd, struct sockaddr *addr, socklen_t addrlen);
void tv_sub(struct  timeval *out, struct timeval *in); //calculate the time interval between out and in

int main(int argc, char *argv[]) {
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
    struct sockaddr_in addr_in;
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(MYUDP_PORT); // htons() converts port number to big endian form
    if (argc < 2) { printf("Provide IP_addr to send to"); exit(1); }
    char *IP_addr = argv[1];
    addr_in.sin_addr.s_addr = inet_addr(IP_addr); // inet_addr converts IP_addr string to big endian form
    bzero(&(addr_in.sin_zero), 8);
    // Typecast internet socket address to generic socket address
    struct sockaddr *addr = (struct sockaddr *)&addr_in;
    socklen_t addrlen = sizeof(struct sockaddr);

    // Create UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { printf("error in socket"); exit(1); }

    // Send file data to socket
    sendtosocket(sockfd, addr, addrlen);
    close(sockfd);
}

void sendtosocket(int sockfd, struct sockaddr *addr, socklen_t addrlen) {
    // Open file for reading
    char filename[] = "myfile.txt";
    FILE* fp = fopen(filename, "rt");
    if (fp == NULL) { printf("File %s doesn't exist\n", filename); exit(1); }

    // Get the total filesize and allocate filebuffer space accordingly
    fseek(fp, 0, SEEK_END); long filesize = ftell(fp); rewind(fp);
    printf("The file length is %d bytes\n", (int)filesize);
    printf("the data unit is %d bytes\n", PACKLEN);
    char filebuffer[filesize]; // buffer used to contain the entire file
    /* Copy the file contents into filebuffer
    fread(void *ptr,        Buffer address
          size_t size,      Size of each element
          size_t count,     Number of elements to be copied
          FILE *stream      File input stream
          ); */
    fread(filebuffer, 1, filesize, fp);
    filebuffer[filesize]=0x4; // Append the filebuffer contents with the End of Transmission ASCII character
    filesize+=1; // Increase filesize by 1 byte because of the addition of the EOT ASCII character
    fclose(fp);

    // Get start time
    struct timeval timeSend;
    gettimeofday(&timeSend, NULL);

    struct ack_so ack;
    int counter = 0;
    char packet[4*PACKLEN]; // buffer used to contain the outgoing packet
    long fileoffset = 0; // Tracks how many bytes have been sent so far
    while (fileoffset < filesize) {
        int packetsize = (PACKLEN < filesize - fileoffset) ? PACKLEN : filesize - fileoffset;
        printf("packetsize = %d\n", packetsize);
        // Copy the next section of the filebuffer into the packet
        memcpy(packet, (filebuffer + fileoffset), packetsize);
        // Send packet data into socket
        int n = sendto(sockfd, &packet, packetsize, 0, addr, addrlen);
        if (n < 0) printf("error in sending packet\n");
        if (counter % 4 == 0) {
            if ((recvfrom(sockfd, &ack, 2, 0, addr, &addrlen)) == -1) {
                printf("error when receiving\n");
                exit(1);
            } else {
                if (!(ack.num && !ack.len)) {
                    printf("Transmission Error\n");
                    return;
                }
            }
            printf("ACK received, alternating packet numbers\n");
        }
        counter+=1;
        fileoffset += packetsize;
    }

    // Get end time
    struct timeval timeRcv;
    gettimeofday(&timeRcv, NULL);

    // Calculate and print transfer rate
    tv_sub(&timeRcv, &timeSend);
    float time = (timeRcv.tv_sec)*1000.0 + (timeRcv.tv_usec)/1000.0;
    printf("Time(ms) : %.3f, Data sent(byte): %d\nData rate: %.3f (Kbytes/s)\n", time, (int)fileoffset, fileoffset/time);
}

void tv_sub(struct  timeval *out, struct timeval *in) {
    if ((out->tv_usec -= in->tv_usec) <0) {
        --out ->tv_sec;
        out ->tv_usec += 1000000;
    }
    out->tv_sec -= in->tv_sec;
}
