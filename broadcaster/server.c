#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#define SERVER_PORT 5555
#define MAX_LINE 256
#define MAX_PENDING 5

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


pthread_mutex_t my_mutex = PTHREAD_MUTEX_INITIALIZER;
struct registrationTable table[10];
int table_index = 0;
pthread_t threads[2];

/*Method Initialization*/
void *join_handler(struct registrationTable *clientData);
void *chat_multicaster();

int main(int argc, char *argv[]) {
	struct sockaddr_in sin;
	struct sockaddr_in clientAddr;
	struct packet packet_reg;
	struct packet packet_con;
	struct packet packet_data;
	struct packet packet_data_return;
	struct registrationTable client_info;
	char buf[MAX_LINE];
	int s, new_s, len, exit_value;

	int error = pthread_create(&threads[1], NULL, chat_multicaster, NULL);

	/* setup passive open */
	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("tcpserver: socket");
		exit(1);
	}

	/* build address data structure */
	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(SERVER_PORT);

	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("tcpclient: bind");
		exit(1);
	}
	listen(s, MAX_PENDING);


	while (1) {
		if ((new_s = accept(s, (struct sockaddr *)&clientAddr, &len)) < 0) {
			perror("tcpserver: accept");
			exit(1);
		} else {
			printf("\n Client has been connected\n");
            printf("\nClient's port is %d \n", ntohs(clientAddr.sin_port));
		}


		//1st Registration packet receive and Create Join Handler
		if (recv(new_s, &packet_reg, sizeof(packet_reg), 0) < 0) {
			printf("\n Could not receive first registration packet\n");
			exit(1);
		}

		if (ntohs(packet_reg.type) != 121) {
			printf("\nNot the registration code\n");
			exit(1);
		}

		//Puts Registered Machine into Client_info
		pthread_mutex_lock(&my_mutex);

		client_info.port = clientAddr.sin_port;
		client_info.sockid = new_s;
		strcpy(client_info.uName, packet_reg.uName);
		strcpy(client_info.mName, packet_reg.mName);
		pthread_mutex_unlock(&my_mutex);

		printf("* Message Type: %d\n", ntohs(packet_reg.type));
		printf("* User Name: %s\n", client_info.uName);
		printf("* Machine Name: %s\n", client_info.mName);

		//Server Creates Join Handler
		exit_value = pthread_create(&threads[0], NULL, join_handler, &client_info);
		int err = pthread_join(threads[0], &exit_value);
	}
}

/*Join Handler*/
void *join_handler(struct registrationTable *clientData)
{
	struct packet packet_reg;
	struct packet packet_con;
	struct registrationTable client_info;
	int newsock = clientData->sockid;


	// handle second registration packet
	if (recv(newsock, &packet_reg, sizeof(packet_reg), 0) < 0) {
		printf("\nCould not receive 2nd registration packet\n");
		exit(1);
	} else {
        if (ntohs(packet_reg.type) != 121) {
            printf("%s%d\n", "Incorrect registration code. Recieved: ", ntohs(packet_reg.type));
            exit(1);
        } else {
            printf("Received second registration packet\n");
        }
    }


	// handle third registration packet
	if (recv(newsock, &packet_reg, sizeof(packet_reg), 0) < 0) {
		printf("\nCould not receive 3rd registration packet\n");
		exit(1);
	} else {
        if (ntohs(packet_reg.type) != 121) {
            printf("%s%d\n", "Incorrect registration code. Recieved: ", ntohs(packet_reg.type));
            exit(1);
        } else {
            printf("Received third registration packet\n");
        }
    }


	// Update the table
	pthread_mutex_lock(&my_mutex);

	//Puts Registered Machine into Table and Increments Index
	table[table_index].port = clientData->port;
	table[table_index].sockid = newsock;
	strcpy(table[table_index].uName, clientData->uName);
	strcpy(table[table_index].mName, clientData->mName);
	table_index++;

	pthread_mutex_unlock(&my_mutex);

	// Send acknowledgement/confirmation to the client
	packet_con.type = htons(221);

	if (send(newsock, &packet_con, sizeof(packet_con), 0) < 0) {
		printf("\n Send Confirmation Failed\n");
		exit(1);
	} else {
        printf("Sent Confirmtion Packet\n");
    }
}

void *chat_multicaster()
{
	char *filename;
	char text[100];
	ssize_t nread;
	struct packet packet_data;
	int fd, index;
	int seq_num = 1;

	filename = "input.txt";
	fd = open(filename, O_RDONLY, 0);

	// We want the server to be constantly sending, thus while 1
	while (1) {
        // If at least one client is listed...
		while (table_index > 0) {
            // ...read 100 bytes of data from input
			while (read(fd, text, 100)) {
				// Construct the data packet
				packet_data.type = htons(131);
				packet_data.seqNumber = seq_num;
				strcpy(packet_data.data, text);

				// Send data packets to each client listed on the table
				for (int i = 0; i < 10; i++) {
					int newsock = table[i].sockid;
					if (newsock != 0) {
						if (send(newsock, &packet_data, sizeof(packet_data), 0) < 0)
						{
							printf("\nFailed to send multicaster data.\n");
							exit(1);
						}
					}
					pthread_mutex_unlock(&my_mutex);
				}
				seq_num++;
				sleep(2);

				printf("Sent to clients\n");
			}
		}
	}

}
