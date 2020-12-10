#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>
#include <pthread.h>

#define SERVER_PORT 5555
#define MAX_LINE 256


struct packet {
	short seqNumber;
	short type;
	char uName[MAX_LINE];
	char mName[MAX_LINE];
	char data[MAX_LINE];
	int groupNum;
};


void *receive_thread(int *sock) {
	int new_sock = *sock;
	struct packet packet_chat;
    packet_chat.type = htons(131);

	while(1) {
		if (recv(*sock, &packet_chat,sizeof(packet_chat),0) < 0) {
			printf("%s\n", "Could not recieve chat packet. \n ");
			exit(1);
		} else {
			printf("\n%s: %s", packet_chat.uName, packet_chat.data);
		}
	}
}


int main(int argc, char* argv[]) {

	struct packet packet_reg;
	struct packet packet_conf;
	struct packet packet_chat;

	packet_reg.type = htons(121);
	packet_conf.type = htons(221);
	packet_chat.type = htons(131);

	struct hostent *hp;
	struct sockaddr_in sin;
	char *host;
	char buf[MAX_LINE];
	char clientname[MAX_LINE];
	clientname[MAX_LINE];
	int s;
	int len;
	int capacity = 0;
	pthread_t threads[2];

	gethostname(clientname, MAX_LINE);

	if (argc == 4) {
		host = argv[1];
	} else {
		fprintf(stderr, "usage:newclient server\n");
		exit(1);
	}

	hp = gethostbyname(host); //translate host name into peer's IP address

	if (!hp) {
			fprintf(stderr, "unkown host: %s\n", host);
			exit(1);
	}

	// keep an active open socket
	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)  {
			perror("tcpclient: socket");
			exit(1);
	}

	// build address data structure
	bzero((char*)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	sin.sin_port = htons(SERVER_PORT);

	//check to see if the client program has connected to the server.
	if (connect(s,(struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("tcpclient: connect");
		close(s);
		exit(1);
	} else {
		printf("%s\n", "Connection successfully established.");
	}

	// construct registration packet since successful connection
	strcpy(packet_reg.mName, clientname);
	strcpy(packet_reg.uName, argv[2]);
	packet_reg.groupNum = atoi(argv[3]);

	// send three registration packets, init to 1 for easier debug messages
	for (int i = 1; i < 4; i++) {
		if (send(s,&packet_reg,sizeof(packet_reg),0) < 0) {
			printf("%s%d%s\n", "Registration packet ", i,  " failed to send. \n");
			exit(1);
		} else {
			printf("%s\n", "Registration packet has been sent successfully. \n");
		}
	}

	//once the packets have been sent, we wait and receive the acknowledgment from the server
	if (recv(s,&packet_conf,sizeof(packet_conf),0) < 0) {
		printf("%s\n", "Confirmation packet failed to be received. \n");
		exit(1);
	} else {
		printf("%s\n", "Confirmation packet has been received successfully. \n");
	}

	// since the ack was successful, we create the thread
	pthread_create(&threads[0],NULL,(void *)receive_thread,&s);

	// be constantly listening for stdin input, thus while True
	while(1) {

		fgets(buf, sizeof(buf), stdin);
		buf[MAX_LINE-1] = '\0';
		len = strlen(buf) + 1;

		strcpy(packet_chat.data, buf);
		strcpy(packet_chat.uName, argv[2]);

		packet_chat.groupNum = atoi(argv[3]);
		send(s, &packet_chat, sizeof(packet_chat), 0);

	}
}
