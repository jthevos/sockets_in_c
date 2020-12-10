#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


#define SERVER_PORT 5555
#define MAX_LINE 256

struct packet {
    short type;
    char uName[MAX_LINE];
    char mName[MAX_LINE];
    char data[MAX_LINE];
    short seqNumber;
};

struct registrationTable {
    int port;
    int sockid;
    char mName[MAX_LINE];
    char uName[MAX_LINE];
};


int main(int argc, char *argv[]) {

	struct hostent *hp;
	struct sockaddr_in sin;
	struct packet packet_reg;
	struct packet packet_con;
	struct packet packet_data_return;
	char *user_name;
	char *host;
	char buf[MAX_LINE];
	int s;
	int len;

	if (argc == 3) {
		host = argv[1];
		user_name = argv[2];
	} else {
		fprintf(stderr, "usage:newclient server\n");
		exit(1);
	}

	/* translate host name into peer's IP address */
	hp = gethostbyname(host);

	if (!hp) {
		fprintf(stderr, "unkown host: %s\n", host);
		exit(1);
	}

	/* active open */
	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("tcpclient: socket");
		exit(1);
	}

	/* build address data structure */
	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	sin.sin_port = htons(SERVER_PORT);


	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("tcpclient: connect");
		close(s);
		exit(1);
	}

	/*Create Registration Packet*/
	packet_reg.type = htons(121);

    // copy remaining host information to reg packet
	char hostname[255];
	int ret = gethostname(hostname, 255);
	strcpy(packet_reg.mName, hostname);
	strcpy(packet_reg.uName, user_name);

    // send three registration packets
	// start at 1 for clearer debug message printing in else block
	for (int i = 1; i < 4; i++) {
		if(send(s,&packet_reg,sizeof(packet_reg),0) < 0) {
			printf("%s\n", "Registration packet failed to send. \n");
			exit(1);
		}
		else {
			printf("%s%d%s\n", "Registration packet ", i, " has been sent successfully. \n");
		}
	}


	/*Receives the Confirmation Code*/
	if (recv(s, &packet_con, sizeof(packet_con), 0) < 0) {
		printf("%s\n", "Confirmation packet failed to be received. \n");
		exit(1);
	}

    // type check confirm code
	if (ntohs(packet_con.type) != 221) {
		printf("%s\n", "Incorrect confirmation code\n");
		printf("%d\n", ntohs(packet_con.type));
		exit(1);
	}

    // print data being sent from server
	while (recv(s, &packet_data_return, sizeof(packet_data_return), 0) > 0) {
		if (ntohs(packet_data_return.type) != 131) {
			printf("Wrong Data Packet Type\n");
			exit(1);
		} else {
            printf("Received Data Seq: %d\n", packet_data_return.seqNumber);
    		printf("%s\n", packet_data_return.data);
        }
	}

}
