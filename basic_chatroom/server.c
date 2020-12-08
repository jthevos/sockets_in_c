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
struct packet //packet structure for all the packets we are sending and receiving
{
	short seqNumber;
	short type;
	char uName[MAX_LINE];
	char mName[MAX_LINE];
	char data[MAX_LINE];
	int groupNum;
};
struct registrationTable //table structure to store all the information of the clients who are connecting to the server
{
	short seqNumber;
	int port;
	int sockid;
	char mName[MAX_LINE];
	char uName[MAX_LINE];
	int groupNum;
};
struct packet buffer[256];
struct registrationTable client_info[10];

//join handler thread that receives registration packets and inputs the client's data into the table
void *join_handler(struct registrationTable *clientData)
{
	//field of variables that are used in the join handler thread
	int newsock;
	newsock = clientData->sockid;
	struct sockaddr_in sin;
	struct sockaddr_in clientAddr;
	bzero((char*)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(SERVER_PORT);
	struct packet packet_reg;
	packet_reg.type = htons(121);
	struct packet packet_conf;
	packet_conf.type = htons(221);
	struct packet packet_chat;
	packet_chat.type = htons(131);

	if(recv(newsock,&packet_reg,sizeof(packet_reg),0)<0) //receiving the second registration packet from the client
	{
		printf("\n Could not receive\n");
		exit(1);
	}
	else
	{
		printf("\n Client has been registered\n"); //receiving the third registration packet from the client
	}
	if(recv(newsock,&packet_reg,sizeof(packet_reg),0)<0)
	{
		printf("\n Could not receive\n");
		exit(1);
	}
	else
	{
		printf("\n Client has been registered\n");
	}
	if(send(newsock,&packet_conf,sizeof(packet_conf),0)<0)
	{
		printf("fail");
		exit(1);
	}
	else
	{
	printf("\n Confirmation packet has been sent\n");
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
	//when the two registration packets are received from the client, then the join handler will send
	//an acknowledgement packet

	while(1)
	{
		if(recv(newsock, &packet_chat,sizeof(packet_chat),0) < 0)
		{
			printf("No");
			exit(1);
		}
		else
		{
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
void *chat_multicaster()
{
	//different variables that are used in the chat multicaster
	struct packet temp;
	int groupNum;
	int i;
	int s;
	int nread;
	struct packet packet_chat;
	packet_chat.type = htons(131);
	//infinity loop so that the text file will not stop sending
	while(1)
	{
	//this while loop will only send if there is atleast one client connect, and will constantly
	//check to see when a client had finally connected
		if(clients > 1)
		{

			//store the packet at an index into a temp packet
			if(bufferindex < arrayIndex)
			{
				pthread_mutex_lock(&buffer_mutex);

				temp.groupNum = buffer[bufferindex].groupNum;
				strcpy(temp.data, buffer[bufferindex].data);
				strcpy(temp.uName, buffer[bufferindex].uName);

				pthread_mutex_unlock(&buffer_mutex);
				pthread_mutex_lock(&table_mutex);

				for ( i = 1; i < clients; i++) //a for loop that will go through the client info table and send the chat data to everone who is in the table
				{
					s = client_info[i].sockid;
					if(client_info[i].groupNum == temp.groupNum)
					{
						if(send(s, &temp,sizeof(temp),0)<0)
						{
							printf("Chat packet was not received \n");
							exit(1);
						}
						else
						{
							printf("sent to: %s ", client_info[i].uName);
						}
					}
				}
				printf("\n%s: %s ", temp.uName, temp.data);
				bufferindex++;
				//unlock the table so join handler can access
				pthread_mutex_unlock(&table_mutex);
			}
		}
	}
}

int main(int argc, char* argv[])
{
	//declare variables and initialize the types of the packets used
	struct packet packet_reg;
	packet_reg.type = htons(121);
	void *exit_value;
	struct sockaddr_in sin;
	struct sockaddr_in clientAddr;
	pthread_t threads[2]; 										//we will need two threads on the server
	int s, new_s;
	int len;
	int cindex = 0;

	//create the multicaster
	pthread_create(&threads[1],NULL,chat_multicaster,NULL);


	/* setup passive open */
	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("tcpserver: socket");
		exit(1);
	}
	/* build address data structure */
	bzero((char*)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(SERVER_PORT);

	if(bind(s,(struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		perror("tcpclient: bind");
		exit(1);
	}
	listen(s, MAX_PENDING);

			//checks to see if the client is accepted
	while(1)
	{
	   if((new_s = accept(s, (struct sockaddr *)&clientAddr, &len)) < 0)
		{
			perror("tcpserver: accept");
			exit(1);
		}
		else
		{
			printf("\n Client has been connected\n");

		}
		//accept the first registration packet so we know to make the join handler
		if(recv(new_s,&packet_reg,sizeof(packet_reg),0)<0)
		{
			printf("Registration packet failed to be received");
		}
		else
		{
			printf("\n Client has been registered\n");
			client_info[0].sockid = new_s;
		}
		//creating of the join handler thread in the server
		pthread_create(&threads[0],NULL,(void *)join_handler,&client_info);
	}
}
