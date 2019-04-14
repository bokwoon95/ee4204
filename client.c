#include "headsock.h"

float str_cli1(FILE *fp, int sockfd, struct sockaddr *addr, int addrlen, int *len);
void tv_sub(struct  timeval *out, struct timeval *in); //calculate the time interval between out and in

int main(int argc, char *argv[]) {
	if (argc!= 2) {
		printf("Provide IP address to send to");
		exit(0);
	}

	/*
	gethostbyname returns a hostent struct
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
	printf("addrs=%p is an *array of in_addr *structs\n", addrs);
	printf("*addrs=%p is equal to addrs[0]=%p\n", *addrs, addrs[0]);
	printf("**addrs=%u is equal to *(addrs[0])=%u\n", **addrs, *(addrs[0]));
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

	// Create socket
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
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
	struct sockaddr_in ser_addr;
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(MYUDP_PORT);
	printf("   ser_addr.sin_addr.s_addr address before=%u\n", ser_addr.sin_addr.s_addr);
	printf("   *(addrs[0])=%u\n", *(addrs[0]));
	memcpy(&(ser_addr.sin_addr.s_addr), addrs[0], sizeof(struct in_addr));
	printf("   ser_addr.sin_addr->s_addr address after memcpy=%u\n", ser_addr.sin_addr.s_addr);
	bzero(&(ser_addr.sin_zero), 8);

	int len;
	int ti = str_cli1(stdin, sockfd, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr_in), &len);
	int rt = (len/(float)ti);                                         //caculate the average transmission rate
	printf("Time(ms) : %.3f, Data sent(byte): %d\nData rate: %f (Kbytes/s)\n", ti, (int)len, rt);

	close(sockfd);
	exit(0);
}

float str_cli1(FILE *fp, int sockfd, struct sockaddr *addr, int addrlen, int *len) {
	char sends[MAXSIZE];

	// get start time
	struct timeval sendt;
	gettimeofday(&sendt, NULL);

	printf("Please input a string (less than 50 characters):\n");
	if (fgets(sends, MAXSIZE, fp) == NULL) {
		printf("error input\n");
	}
	sendto(sockfd, &sends, strlen(sends), 0, addr, addrlen);

	// get end time
	struct timeval recvt;
	gettimeofday(&recvt, NULL);

	// get time delta
	tv_sub(&recvt, &sendt);
	float time_inv = 0.0;
	time_inv += (recvt.tv_sec)*1000.0 + (recvt.tv_usec)/1000.0;
	return(time_inv);
}

void tv_sub(struct  timeval *out, struct timeval *in) {
    if ((out->tv_usec -= in->tv_usec) <0)
    {
        --out ->tv_sec;
        out ->tv_usec += 1000000;
    }
    out->tv_sec -= in->tv_sec;
}
