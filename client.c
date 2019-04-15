#include "headsock.h"
#include <arpa/inet.h>

// Function Prototypes
float sendtosocket2(int sockfd, struct sockaddr *addr, socklen_t addrlen, int *len);
void tv_sub(struct  timeval *out, struct timeval *in); //calculate the time interval between out and in

int main(int argc, char *argv[]) {
    /* Create socket config
       struct fields must be sent in network-byte order (big endian)
       struct sockaddr_in {
       short sin_family;          boilerplate=AF_INET
       ushort sin_port;           TCP/UDP port
       struct sin_addr {
           ulong s_addr;          IPv4 address(es) you wish to send to
       }
       unsigned char[8] sin_zero; boilerplate=structure padding(initialize to all 0s)
       } */
    struct sockaddr_in ser_addr;
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(MYUDP_PORT);
    if (argc < 2) {
        printf("Provide IP address to send to");
        exit(1);
    }
    char *IP_addr = argv[1];
    ser_addr.sin_addr.s_addr = inet_addr(IP_addr);
    bzero(&(ser_addr.sin_zero), 8);

    // Create socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { printf("error in socket"); exit(1); }

    // Send file data to socket
    int len;
    float time = sendtosocket2(sockfd, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr_in), &len);
    float transferrate = (len/(float)time);
    printf("Time(ms) : %.3f, Data sent(byte): %d\nData rate: %f (Kbytes/s)\n", time, (int)len, transferrate);

    // Close socket
    close(sockfd);
    exit(0);
}

float sendtosocket2(int sockfd, struct sockaddr *addr, socklen_t addrlen, int *len) {
    // Open file for reading
    char filename[] = "myfile.txt";
    FILE* fp = fopen(filename, "rt");
    if (fp == NULL) { printf("File %s doesn't exist\n", filename); exit(1); }

    // Get the total filesize and allocate filebuffer space accordingly
    fseek(fp, 0, SEEK_END); long filesize = ftell(fp); rewind(fp);
    printf("The file length is %d bytes\n", (int)filesize);
    printf("the data unit is %d bytes\n", PACKLEN);
    char filebuffer[filesize];
    /* Copy the file contents into filebuffer
    fread(void *ptr,        Buffer address
          size_t size,      Size of each element
          size_t count,     Number of elements to be copied
          FILE *stream      File input stream
          );
    */
    fread(filebuffer, 1, filesize, fp);
    filebuffer[filesize]=0x4; // Append the filebuffer with the End of Transmission ASCII character
    filesize+=1; // Increase filesize by 1 byte because of the addition of the EOT ASCII character
    fclose(fp);

    // Get start time
    struct timeval timeSend;
    gettimeofday(&timeSend, NULL);

    struct ack_so ack;
    int counter = 0;
    char sendbuffer[4*PACKLEN];
    long fileoffset = 0; // Tracks how many bytes have been sent so far
    while (fileoffset < filesize) {
        int packetsize = (PACKLEN < filesize - fileoffset) ? PACKLEN : filesize - fileoffset;
        printf("packetsize = %d\n", packetsize);
        memcpy(sendbuffer, (filebuffer + fileoffset), packetsize);
        int n = sendto(sockfd, &sendbuffer, packetsize, 0, addr, addrlen);
        if (n < 0) printf("error in sending packet\n");
        if (counter % 4 == 0) {
            if ((recvfrom(sockfd, &ack, 2, 0, addr, &addrlen)) == -1) {
                printf("error when receiving\n");
                exit(1);
            } else {
                if (!(ack.num && !ack.len)) {
                    printf("Transmission Error\n");
                    return -1;
                }
            }
            printf("ACK received, alternating packet numbers\n");
        }
        counter+=1;
        fileoffset += packetsize;
    }
    *len = fileoffset;

    // Get end time
    struct timeval timeRcv;
    gettimeofday(&timeRcv, NULL);
    tv_sub(&timeRcv, &timeSend);
    float time_t = 0.0;
    time_t += (timeRcv.tv_sec)*1000.0 + (timeRcv.tv_usec)/1000.0;
    return time_t;
}

void tv_sub(struct  timeval *out, struct timeval *in) {
    if ((out->tv_usec -= in->tv_usec) <0)
    {
        --out ->tv_sec;
        out ->tv_usec += 1000000;
    }
    out->tv_sec -= in->tv_sec;
}
