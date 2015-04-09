#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>

#define BUFLEN 256
#define LOGIN 1
#define LOGOUT 2
#define JOIN 3
#define LEAVE 4
#define CREATE 5
#define LIST 6
#define QUIT 7
#define MAX_NAME 256
#define MAX_DATA 1024

char message [MAX_DATA];
char msg [MAX_DATA];//need a copy of message since strtok is used on message
char *command;
char *ClientID;
char *PWD;
char *ServerIP;
char *ServerPort;
char *SessionID;
bool terminate=0;
int port;
int done =0;
char session_id[100];
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

struct message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};

void *reading(void *vargp) ;
void *writing(void *vargp) ;
int flush();
int get_SID(char* msg_);
int login_parser(char* msg_);
int get_command(char* msg_);
int recvline(const int sock, char *buf, const size_t buflen);
int sendall(const int sock, const char *buf, const size_t len);

int main() {
    /* buffer length */
    struct lab3message packet;
    int n, bytes_to_read;
    int sd;
    struct hostent *hp;
    struct sockaddr_in server;
    char *bp, rbuf[BUFLEN], sbuf[BUFLEN];
    pthread_t tid;
    int i= 0; 
    /* Create a stream socket */
    
    while (!terminate){
        
        printf(">>");
        
        fgets(message,sizeof(message),stdin);
        if(message[strlen(message) - 1] == '\n')//replacement of gets() which is unsafe
            message[strlen(message) - 1] = '\0';
        
        strcpy(msg,message);//need a copy of message since using strtok
        int selection=get_command(message);
        
        int id ;
        int error ;
        char buf[1000];
        
        switch(selection){//handlers
            case LOGIN:
                login_parser(msg);
                printf("Logging In...\n");
                
                if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                    fprintf(stderr, "Can't create a socket\n");
                    exit(1);
                }
                bzero((char *)&server, sizeof(struct sockaddr_in));
                server.sin_family = AF_INET;
                server.sin_port = htons(port);
                if ((hp = gethostbyname(ServerIP)) == NULL) {
                    fprintf(stderr, "Can't get server's address\n");
                    exit(1);
                }
                
                bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
                
                /* Connecting to the server */
                int *connfdp = malloc(sizeof(int));
                *connfdp =sd;
                if ((connect(sd, (struct sockaddr *)&server, sizeof(server))) == -1) {
                    fprintf(stderr, "Can't connect\n");
                    exit(1);
                }
                
                printf("Connected: server's address is %s %d\n", hp->h_name, sd);
                memset(buf, 0, sizeof buf);
		
                snprintf(buf, sizeof buf, "LOGIN %s %s\n", ClientID, PWD);
                if (sendall(sd, buf, strlen(buf)) == 0 && recvline(sd, buf, sizeof buf)
                    == 0) {
                    error = atoi(buf);
                    if(error != 0){
                        printf("Fail\n");
                    }
                    else{
                        id = ClientID;
                        printf("Successful\n");
                    }
                }
                break;
                
            case JOIN:
                get_SID(msg);
                printf("Joining Session...\n");
                memset(buf, 0, sizeof buf);
                snprintf(buf, sizeof buf, "JOINS %s\n",SessionID);
                
                if (sendall(sd, buf, strlen(buf)) == 0 && recvline(sd, buf, sizeof buf)
                    == 0) {
                    error = atoi(buf);
                    if(error != 0){
                        printf("Fail\n");
                    }
                    else{
                        printf("Successful\n");
                        int chatting =1;
                        printf("Welcome to meeting session %s\n",SessionID);
                        done =0;
                        pthread_create(&tid, NULL, writing, (void *) connfdp);
                        pthread_join(tid, NULL);
                    }
                }
                break;
                
            case CREATE:
                get_SID(msg);
                printf("Creating Session...\n");
                memset(buf, 0, sizeof buf);
                snprintf(buf, sizeof buf, "NEWSE %s\n",SessionID);
                if (sendall(sd, buf, strlen(buf)) == 0 && recvline(sd, buf, sizeof buf)
                    == 0) {
                    error = atoi(buf);
                    if(error != 0){
                        printf("Fail\n");
                    }
                    else{
                        printf("Successful\n");
                        int chatting =1;
                        printf("Welcome to meeting session %s\n",SessionID );
                        done =0;
                        pthread_create(&tid, NULL, writing, (void *) connfdp);
                        pthread_join(tid, NULL);
                    }
                }
                break;
                
            case LOGOUT:
                printf("Logging Out...\n");
                memset(buf, 0, sizeof buf);
                snprintf(buf, sizeof buf, "EXITS\n",SessionID);
                sendall(sd, buf, strlen(buf));
                break;
                
            case LEAVE:
                printf("Leaving Session...\n");
                break;
                
            case LIST:  // query
                memset(buf, 0, sizeof buf);
                snprintf(buf, sizeof buf, "QUERY\n",SessionID);
                
                if (sendall(sd, buf, strlen(buf)) == 0 && recvline(sd, buf, sizeof buf)
                    == 0) {
                     
                        int chatting =1;
                        printf("%s\n",buf );
                    
                }
                break;
                
            case QUIT:
                printf("Program Terminated.\n");
                terminate =1;
                break;
                
            case -1:
                printf("Invalid Command.\n");
                break;
                
            default:
                printf("Something is wrong. Re-enter your command.\n");
                break;
                
        }//end of switch
        
    }//end of while
    close(sd);
    return 0;
}

void *reading(void *vargp) {
 	int clientsock = *((int *) vargp);
    free(vargp);
    while(done==0){
        char buf[1000];
        memset(buf, 0, sizeof buf);
        recvline(clientsock, buf, sizeof buf);
        printf("%s\n",buf );
    }
}

void *writing(void *vargp) {
    int clientsock = *((int *) vargp);
    pthread_t tid;
    int *connfdp = malloc(sizeof(int));
    *connfdp =clientsock;
    pthread_create(&tid, NULL, reading,  (void *) connfdp);
    while(done ==0){
        char wbuf[1000];
        memset(wbuf, 0, sizeof wbuf);
        scanf("%[^\n]s",wbuf);
        flush();
        if(strcmp(wbuf,"/leavesession")==0){
            pthread_mutex_lock(&mutex1);
            done =1;
            pthread_mutex_unlock(&mutex1);
            char buf[1000];
            memset(buf, 0, sizeof buf);
            snprintf(buf, sizeof buf, "LEAVE %s\n",SessionID);
            sendall(clientsock, buf, strlen(buf));
            pthread_join(tid,NULL);
        }
        else{
            char buf[1000];
            memset(buf, 0, sizeof buf);
            snprintf(buf, sizeof buf, "MESSA %s %s\n",SessionID,wbuf);
            sendall(clientsock, buf, strlen(buf));
        }
        
    }
}

int sendall(const int sock, const char *buf, const size_t len) {
    size_t tosend = len;
    while (tosend > 0) {
        ssize_t bytes = send(sock, buf, tosend, MSG_NOSIGNAL);
        if (bytes <= 0)
            break; // send() was not successful, so stop.
        tosend -= (size_t) bytes;
        buf += bytes;
    };
    
    return tosend == 0 ? 0 : -1;
}

int recvline(const int sock, char *buf, const size_t buflen) {
    int status = 0; 
    size_t bufleft = buflen;
    
    while (bufleft > 1) {
        // Read one byte from scoket.
        ssize_t bytes = recv(sock, buf, 1, MSG_NOSIGNAL);
        if (bytes <= 0) {
            status = -1;
            break;
        } else if (*buf == '\n') {
            // Found end of line, so stop.
            *buf = 0; // Replace end of line with a null terminator.
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


int get_command(char* msg_){//parse the command/operation
    command=strtok(msg_," ");
    if (!strcmp(command,"/login")) {
        return LOGIN;
    }
    else if (!strcmp(command,"/logout")) {
        return LOGOUT;
    }
    else if (!strcmp(command,"/joinsession")) {
        return JOIN;
    }
    else if (!strcmp(command,"/leavesession")) {
        return LEAVE;
    }
    else if (!strcmp(command,"/createsession")) {
        return CREATE;
    }
    else if (!strcmp(command,"/list")) {
        return LIST;
    }
    else if (!strcmp(command,"/quit")) {
        return QUIT;
    }
    return -1;
}

int login_parser(char* msg_){//parser for login
    
    char* temp;
    temp=strtok(msg_," ");
    
    
    ClientID=strtok(NULL," ");
    if(ClientID)
        printf("Client ID: %s\n",ClientID);
    else printf("Missing: Client ID\n");
    
    PWD=strtok(NULL," ");
    if(PWD)
        printf("PASSWORD: %s\n",PWD);
    else printf("Missing: PASSWORD\n");
    
    ServerIP=strtok(NULL," ");
    if(ServerIP)
        printf("Server IP: %s\n",ServerIP);
    else printf("Missing: Server IP\n");
    
    ServerPort=strtok(NULL," ");
    if(ServerPort){
        printf("Server Port: %s\n",ServerPort);
    port = atoi(ServerPort);}
    else printf("Missing: Server Port\n");
    
    return 0;
}

int get_SID(char* msg_){//pareser for join and create
    
    char* temp;
    temp=strtok(msg_," ");
    
    SessionID=strtok(NULL," ");
    if(SessionID)
        printf("Session ID: %s\n",SessionID);
    else printf("Missing: Session ID\n");
    
    return 0;
}

int flush(){
    int ch;
    while ((ch = getchar()) != EOF && ch != '\n') ;
    return 0;
}
