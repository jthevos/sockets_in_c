#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>


#define SERVER_PORT 5555
#define MAX_LINE 256

struct packet {
    short type;
    char uName[MAX_LINE];
    char mName[MAX_LINE];
    char data[MAX_LINE];
};

struct registrationTable {
    int port;
    int sockid;
    char mName[MAX_LINE];
    char uName[MAX_LINE];
};


int main(int argc, char* argv[]) {

	struct hostent *hp;
	struct sockaddr_in sin;
	struct packet packet_reg;
	struct packet packet_con;
	struct packet packet_data;
	struct packet packet_data_return;
	char *user_name;
	char *host;
	char buf[MAX_LINE];
	int s;
	int len;

	if (argc == 3) {
		host = argv[1];
		user_name = argv[2];
	}
	else {
		fprintf(stderr, "usage:newclient server\n");
		exit(1);
	}

	/* translate host name into peer's IP address */
	hp = gethostbyname(host);
	if (!hp) {
		fprintf(stderr, "unkown host: %s\n", host);
		exit(1);
	}

	/* setup active open */
	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("tcpclient: socket");
		exit(1);
	}

	/* build address data structure */
	bzero((char*)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	sin.sin_port = htons(SERVER_PORT);


	if (connect(s,(struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("tcpclient: connect");
		close(s);
		exit(1);
	}

	/*Create Registration Packet*/
	packet_reg.type = htons(121);

	char hostname[255];
	int ret = gethostname(hostname, 255);
	strcpy(packet_reg.mName, hostname);
	strcpy(packet_reg.uName, user_name);

	/*Send Registration code*/
	printf("Sending Registration Packet: Type %d", 121);
	if (send(s, &packet_reg, sizeof(packet_reg), 0) < 0) {
		printf("\n Send failed\n");
		exit(1);
	}

	/*Receives the Confirmation Code*/
	if (recv(s, &packet_con,sizeof(packet_con), 0) < 0) {
		printf("\n Could not receive confirmation packet\n");
		exit(1);
	} else {
		if (ntohs(packet_con.type) != 221) {
			printf("\nNot the confirmation code\n");
			exit(1);
		} else {
			printf("%s\n","successfully recieved confirmation packet.");
		}
	}

	// Wait for stdin input and send to the server
	while(fgets(buf, sizeof(buf), stdin)) {
		buf[MAX_LINE-1] = '\0';

		// create data packet
		packet_data.type = htons(131);
		strcpy(packet_data.uName, user_name);
		strcpy(packet_data.data, buf);

		// send data packets
		if (send(s, &packet_data, sizeof(packet_data), 0) < 0) {
			printf("\nCould not send packet data\n");
			exit(1);
		}

		// handle receipt of packets from the server
		if (recv(s, &packet_data_return, sizeof(packet_data_return), 0) < 0) {
			printf("\nCould not receive data return\n");
			exit(1);
		}

		printf("Chat Response Received: Packet Type %d\n", ntohs(packet_data_return.type));
	}
}
