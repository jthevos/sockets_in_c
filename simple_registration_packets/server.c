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
#define MAX_PENDING 5

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

	struct sockaddr_in sin;
	struct sockaddr_in clientAddr;
	struct packet packet_reg;
	struct packet packet_con;
	struct packet packet_data;
	struct packet packet_data_return;
	struct registrationTable table[10];
	int table_index = 0;
	char buf[MAX_LINE];
	int s, new_s;
	int len;


	/* setup passive open socket for incoming connections */
	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("tcpserver: socket");
		exit(1);
	}

	/* build address data structure */
	bzero((char*)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(SERVER_PORT);


	if (bind(s,(struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("tcpclient: bind");
		exit(1);
	}

	listen(s, MAX_PENDING);

	/* Always await connections, then receive and print text */
	while(1) {
		if ((new_s = accept(s, (struct sockaddr *)&clientAddr, &len)) < 0) {
			perror("tcpserver: accept");
			exit(1);
		}

		//Registration packet receive
		if (recv(new_s,&packet_reg,sizeof(packet_reg),0) < 0) {
			printf("\n Could not receive first registration packet\n");
			exit(1);
		}

		if (ntohs(packet_reg.type) != 121) {
			printf("%s%d\n", "Not the registration code. Recieved: ", ntohs(packet_reg.type));
			exit(1);
		}

		//Puts Registered Machine into Table
		table[table_index].port = clientAddr.sin_port;
		table[table_index].sockid = new_s;
		strcpy(table[table_index].uName, packet_reg.uName);
		strcpy(table[table_index].mName, packet_reg.mName);

        printf("* Message Type: %d\n", ntohs(packet_reg.type));
        printf("* User Name: %s\n", table[table_index].uName);
        printf("* Machine Name: %s\n", table[table_index].mName);

        // Incrment table index
		table_index++;

		//Server Sends Confirmation Packet
		packet_con.type = htons(221);

		if (send(new_s, &packet_con, sizeof(packet_con), 0) < 0) {
			printf("\n Send Confirmation Failed\n");
			exit(1);
		}

		while(recv(new_s, &packet_data, sizeof(packet_data), 0)) {
			char user_text[256];

			//print username and message next to each other.
			snprintf(user_text, sizeof(user_text), "[%s]: %s", packet_data.uName, packet_data.data);
			fputs(user_text, stdout);

            // construct return packet
			packet_data_return.type = htons(231);
			strcpy(packet_data_return.uName, packet_data.uName);
			strcpy(packet_data_return.data, packet_data.data);

			//send data back to client
			if (send(new_s, &packet_data_return, sizeof(packet_data_return), 0) < 0) {
				printf("\nFailed to send data return\n");
				exit(1);
			}
		}
		close(new_s);
	}
}
