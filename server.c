#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <pthread.h>

#define BUFLEN 1024
#define MAX_CLIENT 20
#define MAX_MESSAGE_LENGTH 1000
#define MAX_PASSWORD 12
#define MAX_NAME 256
#define MAX_DATA 1024

struct lab3message {
 unsigned int type;
 unsigned int size; 
 unsigned char source[MAX_NAME];
 unsigned char data[MAX_DATA];
}; 

struct conference
{	
	char s_id[MAX_NAME];
	int client_id[MAX_CLIENT];
	int client_Sockets[MAX_CLIENT];
	char message[MAX_MESSAGE_LENGTH];
	int flag;
	int count;
};

struct  client
{
	char password[7];
};

struct conference conference[21];
struct client client[21];

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
void *do_server_request(void *vargp);
void preset_data();


int main(int argc, char *argv[]) {
	
    int n, bytes_to_read;
    int sd, new_sd, client_len, port; 
    struct sockaddr_in server, client; 
    char *bp, buf[BUFLEN];
   	pthread_t tid;

    if (argc != 2) {
       fprintf(stderr,"usage: %s <server port number> \n", argv[0]);
       exit(0);
    }

    /* Create a stream socket */
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Can't create a socket\n");
        exit(1); 
    }

    port = atoi(argv[1]);
    /* Bind an address to the socket */
    bzero((char *)&server, sizeof(struct sockaddr_in)); 
    server.sin_family = AF_INET;
    server.sin_port = htons(port); 
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        fprintf(stderr, "Can't bind name to socket\n");
        exit(1); 
    }
    preset_data();
    listen(sd, 20);

    while (1) {
        client_len = sizeof(client);
        int *connfdp = malloc(sizeof(int));
        if ((*connfdp = accept(sd, (struct sockaddr *) &client, &client_len)) == -1) {
        fprintf(stderr, "Can't accept client\n");
        exit(1); 
        }
		pthread_create(&tid, NULL, do_server_request, (void *) connfdp);  
    }

}

void *do_server_request(void *vargp) {
	int clientsock;
	char bp[BUFLEN];
	int size;
	int n, bytes_to_read;

	pthread_detach(pthread_self());
	clientsock = *((int *) vargp);
	free(vargp);
	int id;
	// Get commands from client.
	int wait_for_commands = 1;
	do {
		printf("wait_for_commands %d\n",clientsock);
		recvline(clientsock, bp, BUFLEN);
		printf("----%s-----\n",bp);
		char check[8];
		strncpy(check, bp, 5);
		check[5] = '\0';
		int error = 0;
		char userid [MAX_NAME];
		char password [MAX_PASSWORD];
		char temp[8];
		
		if (!strcmp(check, "LOGIN")) {
			
			int items = sscanf(bp, "%s %s %s\n",temp ,userid, password);
			id = atoi (userid);
			printf("id %d\n",id );
			if(items!= 3){
				error = 1;
			}
			else if(id <0 || id > 20) {
				error = 1;
			}
			else if (strcmp(password,client[id].password)!=0){
				error = 2;
			}
				
				char cccmmmddd[MAX_DATA];
				snprintf(cccmmmddd, sizeof cccmmmddd, "%d",error);
				sendall(clientsock, cccmmmddd, strlen(cccmmmddd));
				sendall(clientsock, "\n", 1);
			
		}	

		else if(!strcmp(check, "NEWSE")){

			char session_id[MAX_NAME];
			int items = sscanf(bp, "%s %s\n",temp,session_id);
			int i =0;
			while((conference[i].flag ==1)&&(i<21)){
				i++;
			}
			if(i>20)
				error =3 ;	// 20 sessions are occupied
			if(error ==0){
				pthread_mutex_lock(&mutex1);
				strcpy(conference[i].s_id,session_id);
				conference[i].client_id[0]= id;
				int j =1;
				while(j>=20){
					conference[i].client_id[j] =0;
					conference[i].client_Sockets[j] =-1;
					j++;
				}
				printf("conference %d %d\n",id,i);
				conference[i].client_Sockets[0] =clientsock;
				conference[i].client_Sockets[1] =-1;
				conference[i].flag = 1;
				conference[i].count = 1;
				pthread_mutex_unlock(&mutex1);
			}
		

			char cccmmmddd[MAX_DATA];
			snprintf(cccmmmddd, sizeof cccmmmddd, "%d %s", error, session_id);
			sendall(clientsock, cccmmmddd, strlen(cccmmmddd));
			sendall(clientsock, "\n", 1);
		}

		else if(!strcmp(check, "JOINS")){
			char session_id[MAX_NAME];
			int items = sscanf(bp, "%s %s\n",temp,session_id);
			int i =0;
			while(strcmp(session_id, conference[i].s_id)!=0&&(i<19)){ 
				i++;
			}
			if(conference[i].flag ==0){
				error =5;
			}
			if(i>19)
				error =4; // don't find correct positon
			int j =0;
			if(error==0){
				while((conference[i].client_id[j] !=0)&&(j<21)){
					j++;
				}
				if(j>20)
					error =5; // meeting session is full; 
				if(error ==0){
					pthread_mutex_lock(&mutex1);
					conference[i].client_id[j] = id;
					conference[i].client_Sockets[j] = clientsock;
					conference[i].count++;
					pthread_mutex_unlock(&mutex1);
				}
			}
			char cccmmmddd[MAX_DATA];
			snprintf(cccmmmddd, sizeof cccmmmddd, "%d %s", error, session_id);
			sendall(clientsock, cccmmmddd, strlen(cccmmmddd));
			sendall(clientsock, "\n", 1);
		}

		else if(!strcmp(check, "LEAVE")){
			char session_id[MAX_NAME];

			int items = sscanf(bp, "%s\n",session_id);
			int i =0;
			while(strcmp(session_id, conference[i].s_id)==0){
				i++;
			}
			int j =0;
			pthread_mutex_lock(&mutex1);
			while((conference[i].client_id[j] != id)&&(j<21)){
				j++;
			}

			conference[i].client_id[j] =0;
			conference[i].client_Sockets[j] =-1;
			conference[i].count--;
			if(conference[i].count ==0){
				conference[i].flag=0;
				int k =0;
				while(k>=20){
					conference[i].client_id[k] =0;
					conference[i].client_Sockets[k] =-1;
					k++;
				}
			}
			pthread_mutex_unlock(&mutex1);
			char cccmmmddd[MAX_DATA];
			snprintf(cccmmmddd, sizeof cccmmmddd, "/quit");
			printf("quit %d %d %d\n",i,id,conference[i].count);
			sendall(clientsock, cccmmmddd, strlen(cccmmmddd));
			sendall(clientsock, "\n", 1);
		}

		else if(!strcmp(check, "MESSA")){
			char session_id[MAX_NAME];
			char message[MAX_DATA];
			int items = sscanf(bp, "%s %s %[^\n]",temp,session_id,message);

			int i =0;
			while(strcmp(session_id, conference[i].s_id)!=0){
				i++;
			}
			int j =0;
			
			while(j<20){
				if((conference[i].client_id[j] != id) &&(conference[i].client_id[j]!=0)){
					char cccmmmddd[MAX_DATA];
					snprintf(cccmmmddd, sizeof cccmmmddd, "%d: %s",id,message);
					sendall(conference[i].client_Sockets[j], cccmmmddd, strlen(cccmmmddd));
					sendall(conference[i].client_Sockets[j], "\n", 1);
				}
				j++;
			}
		}
		else if(!strcmp(check, "QUERY")){
			int i =0;
			char result[MAX_DATA];
			int length = 0;
			int k =0;
			while(i<20){
				printf("%d\n",i);
				if(conference[i].flag == 1){
					int j =0;
					while(j<20){
						if(conference[i].client_id[j] != 0){
							k=1;
							length += sprintf(result + length, "%s %d ",
									conference[i].s_id, conference[i].client_id[j]);
							}
						j++;
					}
				}
				i++;
			}
			char cccmmmddd[MAX_DATA];
			if(k==1)
				snprintf(cccmmmddd, sizeof cccmmmddd, "%s", result);
			else
				snprintf(cccmmmddd, sizeof cccmmmddd, "Empty
");
			sendall(clientsock, cccmmmddd, strlen(cccmmmddd));
			sendall(clientsock, "\n", 1);

		}

		else if(!strcmp(check, "EXITS")){
			wait_for_commands =0;
		}
		
	} while (wait_for_commands);

	// Close the connection with the client.
	close(clientsock);
	return NULL ;
}

int sendall(const int sock, const char *buf, const size_t len) {
	size_t tosend = len;
	while (tosend > 0) {
		ssize_t bytes = send(sock, buf, tosend, 0);
		if (bytes <= 0)
			break; // send() was not successful, so stop.
		tosend -= (size_t) bytes;
		buf += bytes;
	};

	return tosend == 0 ? 0 : -1;
}

int recvline(const int sock, char *buf, const size_t buflen) {
	int status = 0; // Return status.
	size_t bufleft = buflen;

	while (bufleft > 1) {
		// Read one byte from scoket.
		ssize_t bytes = recv(sock, buf, 1, 0);
		if (bytes <= 0) {
			status = -1;
			break;
		} else if (*buf == '\n') {
			*buf = 0; 
			status = 0;
			break;
		} else {
			bufleft -= 1;
			buf += 1;
		}
	}
	*buf = 0; 
	return status;
}

void preset_data(){
	int i;
	for(i=0;i<21;i++){
		strcpy(client[i].password,"abcd"); 
		conference[i].flag =0;
	}

}


