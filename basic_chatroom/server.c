#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>
#include<pthread.h>
#include<fcntl.h>
#include<unistd.h>

#define SERVER_PORT 5555
#define MAX_LINE 256
#define MAX_PENDING 5

int clients = 1; // clients represents the number of clients connected
int bufferindex = 0;
int arrayIndex = 0;
pthread_mutex_t table_mutex = PTHREAD_MUTEX_INITIALIZER;		// global mutex variable
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

//packet structure for all the packets we are sending and receiving
struct packet {
	short seqNumber;
	short type;
	char uName[MAX_LINE];
	char mName[MAX_LINE];
	char data[MAX_LINE];
	int groupNum;
};

//table structure to store all the information of the clients who are connecting to the server
struct registrationTable {
	short seqNumber;
	int port;
	int sockid;
	char mName[MAX_LINE];
	char uName[MAX_LINE];
	int groupNum;
};

struct packet buffer[256];
struct registrationTable client_info[10];

//join handler thread.
//Receives registration packets and inputs the client's data into the master connection table
void *join_handler(struct registrationTable *clientData) {

	int newsock;
	newsock = clientData->sockid;
	struct sockaddr_in sin;
	struct sockaddr_in clientAddr;

	// Construct address information again
	bzero((char*)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(SERVER_PORT);

	struct packet packet_reg;
	struct packet packet_conf;
	struct packet packet_chat;

	packet_reg.type = htons(121);
	packet_conf.type = htons(221);
	packet_chat.type = htons(131);


	// we recieve the FIRST registration packet in the main function.
	// after that packet is recieve, this join handler is created.

	//receiving the second registration packet from the client
	if(recv(newsock,&packet_reg,sizeof(packet_reg),0)<0) {
		printf("%s\n", "Could not receive registration packet.");
		exit(1);
	}
	else {
		printf("%s\n", "Client has been registered.\n");
	}

	//receiving the third registration packet from the client
	if(recv(newsock,&packet_reg,sizeof(packet_reg),0)<0) {
		printf("%s\n", "Could not receive registration packet.\n");
		exit(1);
	}
	else {
		printf("%s\n", "Client has been registered.\n");
	}

	if(send(newsock,&packet_conf,sizeof(packet_conf),0)<0) {
		printf("%s\n","Unable to send confirmation packet.\n");
		exit(1);
	}
	else {
		printf("%s\n", "Confirmation packet has been sent\n");
	}

	pthread_mutex_lock(&table_mutex);

	//add the client's info into the table
	client_info[clients].port = clientAddr.sin_port;
	client_info[clients].sockid = newsock;
	strcpy(client_info[clients].uName, packet_reg.uName);
	strcpy(client_info[clients].mName, packet_reg.mName);
	client_info[clients].groupNum = packet_reg.groupNum;
	clients++;

	pthread_mutex_unlock(&table_mutex);

	//An acknowledgement packet is sent after 3 registration packets arrive

	while(1)
	{
		if(recv(newsock, &packet_chat,sizeof(packet_chat),0) < 0) {
			printf("Chat packet failed to be received correctly.");
			exit(1);
		}
		else {
			pthread_mutex_lock(&buffer_mutex);
			buffer[arrayIndex].groupNum = packet_chat.groupNum;
			strcpy(buffer[arrayIndex].data, packet_chat.data);
			strcpy(buffer[arrayIndex].uName, packet_chat.uName);
			arrayIndex++;
			pthread_mutex_unlock(&buffer_mutex);
		}
	}
}

//chat multicaster that will send data constantly while there is atleast one client connected.
void *chat_multicaster() {
	//different variables that are used in the chat multicaster
	struct packet temp;
	int groupNum;
	int i;
	int s;
	int nread;
	struct packet packet_chat;
	packet_chat.type = htons(131);

	// we want the multicaster always running, thus While True
	while(1) {

	// but, we only want to send anything if there is a client
		if (clients > 1) {

			//store the packet at an index into a temp packet
			if (bufferindex < arrayIndex) {
				// lock the buffer mutex
				pthread_mutex_lock(&buffer_mutex);

				// update temp packet
				temp.groupNum = buffer[bufferindex].groupNum;
				strcpy(temp.data, buffer[bufferindex].data);
				strcpy(temp.uName, buffer[bufferindex].uName);
				// unlock the buffer mutex
				pthread_mutex_unlock(&buffer_mutex);

				// lock the master mutex
				pthread_mutex_lock(&table_mutex);

				// we need to send the chat data to everyone. We use a for loop
				// for this as we know how many clients are tuned in.
				for (i = 1; i < clients; i++) {
					s = client_info[i].sockid;
					if(client_info[i].groupNum == temp.groupNum) {
						if(send(s, &temp,sizeof(temp),0)<0) {
							printf("Chat packet was not received \n");
							exit(1);
						}
						else {
							printf("sent to: %s ", client_info[i].uName);
						}
					}
				}

				printf("\n%s: %s ", temp.uName, temp.data);
				bufferindex++;

				//unlock the master table so join handler can access
				pthread_mutex_unlock(&table_mutex);
			}
		}
	}
}

int main(int argc, char* argv[]) {

	struct packet packet_reg;
	packet_reg.type = htons(121);

	void *exit_value;
	struct sockaddr_in sin;
	struct sockaddr_in clientAddr;
	pthread_t threads[2]; 			//we will need two threads on the server
	int s, new_s;
	int len;
	int cindex = 0;

	//create the multicaster
	pthread_create(&threads[1],NULL,chat_multicaster,NULL);


	// setup passive open
	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("tcpserver: socket");
		exit(1);
	}

	// build address data structure
	bzero((char*)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(SERVER_PORT);

	if(bind(s,(struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("tcpclient: bind");
		exit(1);
	}

	listen(s, MAX_PENDING); //checks to see if the client is accepted

	while(1) {
	   if((new_s = accept(s, (struct sockaddr *)&clientAddr, &len)) < 0) {
			perror("tcpserver: accept");
			exit(1);
		}
		else {
			printf("%s\n", "Client has been connected.");
		}

		//accept the first registration packet so we know to make the join handler
		if(recv(new_s,&packet_reg,sizeof(packet_reg),0)<0) {
			printf("%s\n", "Registration packet failed to be received");
		}
		else {
			printf("%s\n", "Client has been registered\n");
			client_info[0].sockid = new_s;
		}
		//creating of the join handler thread in the server
		pthread_create(&threads[0],NULL,(void *)join_handler,&client_info);
	}
}
