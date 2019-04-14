#include "headsock.h"

/* float sendtosocket(FILE *fp, int sockfd, struct sockaddr *addr, int addrlen, int *len); */
float sendtosocket2(FILE *fp, int sockfd, struct sockaddr *addr, socklen_t addrlen, int *len);
void tv_sub(struct  timeval *out, struct timeval *in); //calculate the time interval between out and in

int main(int argc, char *argv[]) {
    // Get server IP addr
    if (argc!= 2) {
        printf("Provide IP address to send to");
        exit(0);
    }

    // Create socket
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("error in socket");
        exit(1);
    }

    /* Call gethostbyname to get a hostent struct
       struct hostent {
       char *h_name;        hostname
       char **h_aliases;    alias list
       int h_addrtype;      host address type
       int h_length;        length of host address
       char **h_addr_list;  list of host addresses
       }
       */
    struct hostent *sh;
    if ((sh=gethostbyname(argv[1]))==NULL) {
        printf("error when gethostbyname");
        exit(0);
    }
    struct in_addr **addrs;
    addrs = (struct in_addr **)sh->h_addr_list;
    printf("/*----------DIAGNOSTIC INFO-------------*/\n");
    printf("   addrs=%p is an *array of in_addr *structs\n", addrs);
    printf("   *addrs=%p is equal to addrs[0]=%p\n", *addrs, addrs[0]);
    printf("   (**addrs).s_addr=%u is equal to (*(addrs[0])).s_addr)=%u\n", (**addrs).s_addr, (*(addrs[0])).s_addr);

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
    struct sockaddr_in ser_addr;
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(MYUDP_PORT);
    printf("   ser_addr.sin_addr.s_addr address before memcpy=%u\n", ser_addr.sin_addr.s_addr);
    printf("   (*(addrs[0])).s_addr=%u\n", (*(addrs[0])).s_addr);
    memcpy(&(ser_addr.sin_addr.s_addr), addrs[0], sizeof(struct in_addr));
    printf("   ser_addr.sin_addr->s_addr address after memcpy=%u\n", ser_addr.sin_addr.s_addr);
    printf("/*----------END DIAGNOSTIC INFO----------*/\n");
    bzero(&(ser_addr.sin_zero), 8);
    printf("canonical name: %s\n", sh->h_name);
    char **pptr;
    for (pptr=sh->h_aliases; *pptr != NULL; pptr++)
        printf("the aliases name is: %s\n", *pptr);
    switch(sh->h_addrtype) {
        case AF_INET:
            printf("AF_INET\n");
            break;
        default:
            printf("unknown addrtype\n");
            break;
    }

    // Open file handler
    FILE *fp;
    /* char *filename; */
    /* filename = "smalldata"; */
    /* filename = "datatosend.txt"; */
    if((fp = fopen("datatosend.txt", "r+t")) == NULL) {
        printf("File doesn't exist\n");
        exit(0);
    }

    // Send file data to socket
    int len;
    /* int time = sendtosocket(stdin, sockfd, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr_in), &len); */
    float time = sendtosocket2(fp, sockfd, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr_in), &len);
    float transferrate = (len/(float)time);
    printf("Time(ms) : %.3f, Data sent(byte): %d\nData rate: %f (Kbytes/s)\n", time, (int)len, transferrate);

    // Close socket
    close(sockfd);
    fclose(fp);
    exit(0);
}

float sendtosocket2(FILE *fp, int sockfd, struct sockaddr *addr, socklen_t addrlen, int *len) {
    // Get the total file size of *fp
    long filesize = 0;
    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    rewind(fp);
    printf("The file length is %d bytes\n", (int)filesize);
    printf("the data unit is %d bytes\n", DATALEN);

    // Allocate memory to contain the whole file.
    char *buf;
    buf = (char *) malloc (filesize);
    if (buf == NULL) exit (2);

    // Copy the file into the buffer.
    fread (buf,1, filesize,fp);

    /*** The whole file is loaded in the buffer. ***/
    buf[filesize] ='\0'; //append the end byte

    // Get start time
    struct timeval timeSend;
    gettimeofday(&timeSend, NULL);

    struct ack_so ack;
    long cursor = 0;
    char sends[2*DATALEN];
    int n = 0;
    int sendLength = 0;
    int counter = 0;
    while(cursor <= filesize) {
        counter = counter%3;
        if ((filesize - cursor + 1) < (DATALEN)) {
            sendLength = filesize - cursor + 1;
        } else {
            sendLength = DATALEN;
        }
        printf("slen = %d\n", sendLength);
        memcpy(sends, (buf + cursor), sendLength);
        if ((n = sendto(sockfd, &sends, sendLength, 0, addr, addrlen)) == -1) {
            if (n < 0) {
                printf("error in send\n");
            }
        }
        if(!(counter%2)) {
            if ((n = recvfrom(sockfd, &ack, 2, 0, addr, &addrlen)) == -1) {
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
        cursor += sendLength;
        counter++;
    }

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

/* float sendtosocket(FILE *fp, int sockfd, struct sockaddr *addr, int addrlen, int *len) { */
/*     char sends[2*DATALEN]; */
/*  */
/*     fseek(fp, 0, SEEK_END); */
/*     long filesize = ftell(fp); */
/*     rewind(fp); */
/*     printf("The file length is %d bytes\n", (int)filesize); */
/*     printf("the data unit is %d bytes\n", DATALEN); */
/*  */
/*     // get start time */
/*     struct timeval sendt; */
/*     gettimeofday(&sendt, NULL); */
/*  */
/*     printf("Please input a string (less than 50 characters):\n"); */
/*     if (fgets(sends, MAXSIZE, fp) == NULL) { */
/*         printf("error input\n"); */
/*     } */
/*     sendto(sockfd, &sends, strlen(sends), 0, addr, addrlen); */
/*  */
/*     // get end time */
/*     struct timeval recvt; */
/*     gettimeofday(&recvt, NULL); */
/*  */
/*     // get time delta */
/*     tv_sub(&recvt, &sendt); */
/*     float time_inv = 0.0; */
/*     time_inv += (recvt.tv_sec)*1000.0 + (recvt.tv_usec)/1000.0; */
/*     return(time_inv); */
/* } */
