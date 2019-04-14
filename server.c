#include "headsock.h"


void str_ser1(int sockfd);

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

	printf("start receiving\n");
	while(1) {
		str_ser1(sockfd);
	}

	// Close socket
	close(sockfd);
	exit(0);
}

void str_ser1(int sockfd) {
	char recvs[MAXSIZE];
	int n = 0;
	struct sockaddr_in addr;

	/*
	int recvfrom(
		int socket,
		void *buf,
		size_t buflen,
		int flags,
		struct sockaddr *sender_addr,   boilerplate
		socklen_t *sender_addrlen       boilerplate
		);
		returns length of data read. If 0, connection is closed. If -1, error occurred. recvfrom is BLOCKING by default.
	*/
	int len = sizeof (struct sockaddr_in);
	if ((n=recvfrom(sockfd, &recvs, MAXSIZE, 0, (struct sockaddr *)&addr, &len)) == -1) {
		printf("error receiving");
		exit(1);
	}

	recvs[n] = '\0';
	printf("the received string is :\n%s", recvs);
}
