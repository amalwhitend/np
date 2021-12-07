#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>

#define PORT 1234
#define BACKLOG 1
#define Max 5
#define MAXSIZE 1024

struct userinfo {
    char id[100];
    int playwith;
};


void clear_borad(int *borad){
    for(int i=0;i<9;i++) borad[i]=0;
}

int fdt[Max]={0};
char mesg[1024];
int SendToClient(int fd,char* buf,int Size);
struct userinfo users[100];
int find_fd(char *name);
int win[8][3] = {{0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}};

//find index of this ID
int find_fd(char *name){
  for(int i=0; i<100; i++){
    if(strcmp(name,users[i].id) == 0) return i;
  }
  return -1;
}

void message_handler(char *mesg, int sender){
  int instru = 0;
  sscanf(mesg,"%d",&instru);
  switch(instru){
    //register
    case 1:{
        char name[100];
        sscanf(mesg,"1 %s",name);
        strncpy(users[sender].id,name,100);
        send(sender,"1",1,0);
        printf("1:%s\n",name);
        break;
    }
    //show active
    case 2:{
        char buf[MAXSIZE],tmp[100];
        int p = sprintf(buf,"2 ");
        for(int i=0; i<100; i++)
          if(strcmp(users[i].id,"")!=0){
            sscanf(users[i].id,"%s",tmp);
            p = sprintf(buf+p,"%s ",tmp)+p;
          }
        printf("2:%s\n",buf);
        send(sender,buf,strlen(buf),0);
        break;
    }
    //invite
    case 3:{
        char a[100],b[100];
        char buf[MAXSIZE];
        sscanf(mesg,"3 %s %s",a,b);
        int b_fd = find_fd(b);
        sprintf(buf,"4 %s invite you. Accept?\n",a);
        send(b_fd,buf,strlen(buf),0);
        printf("3:%s",buf);
        break;
    }
    //agree the invitation(1) or not(0)
    case 5:{
      int state;
      char inviter[100];
      sscanf(mesg,"5 %d %s",&state,inviter);
      if(state == 1){
        send(sender,"6\n",2,0);
        int fd = find_fd(inviter);
        send(fd,"6\n",2,0);
        users[sender].playwith = fd;
        users[fd].playwith = sender;
        printf("6:\n");
      }
      break;
    }
    //gaming
    case 7:{
      int board[9];
      char state[100];
      char buf[MAXSIZE];
      sscanf(mesg,"7 %d %d %d %d %d %d %d %d %d",&board[0],&board[1],&board[2],&board[3],&board[4],&board[5],&board[6],&board[7],&board[8]);
      //for(int i=0; i<100; i++) state[i] = '\0';
      //check if somebody win
      memset(buf,'\0',MAXSIZE);
      memset(state,'\0',sizeof(state));
      strcat(state,users[sender].id);
      for(int i=0; i<8; i++){
        if(board[win[i][0]] == board[win[i][1]] && board[win[i][0]] == board[win[i][2]] && board[win[i][0]]!=0){
          strcat(state,"_Win!!\n");
          sprintf(buf,"8 %d %d %d %d %d %d %d %d %d %s\n",board[0],board[1],board[2],board[3],board[4],board[5],board[6],board[7],board[8],state);
          printf("7:%s",buf);
          send(sender,buf,sizeof(buf),0);
          send(users[sender].playwith,buf,sizeof(buf),0);
          return;
        }
      }
      //check if the board fill
      memset(buf,'\0',sizeof(buf));
      memset(state,'\0',sizeof(state));
      for(int i=0; i<9; i++){
        if(board[i] == 0) break;
        if(i == 8){
          strcat(state,"Tie!\n");
          sprintf(buf,"8  %d %d %d %d %d %d %d %d %d %s\n",board[0],board[1],board[2],board[3],board[4],board[5],board[6],board[7],board[8],state);
          printf("7:%s",buf);
          send(sender,buf,sizeof(buf),0);
          send(users[sender].playwith,buf,sizeof(buf),0);
          return;
        }
      }
      //game
      memset(buf,'\0',sizeof(buf));
      memset(state,'\0',sizeof(state));
      strcat(state, users[users[sender].playwith].id);
      strcat(state, "_your_tern!\n");
      sprintf (buf,"8  %d %d %d %d %d %d %d %d %d %s\n",board[0],board[1],board[2],board[3],board[4],board[5],board[6],board[7],board[8],state);
      printf ("7:%s",buf);
      send(sender,buf,sizeof(buf),0);
      send(users[sender].playwith,buf,sizeof(buf),0);
      break;
    }
  }
}

//new thread
void *pthread_service(void* sfd){
  int fd = *(int *)sfd;
  while(1){
    int num;
    int i;
    num = recv(fd,mesg,MAXSIZE,0);
    printf("\n===================\n\n%s\n===================\n\n",mesg);
    //close socket
    if(num <= 0){
      for(int i=0; i<Max; i++){
        if(fd == fdt[i]){
          fdt[i] = 0;
        }
      }
      memset(users[fd].id,'\0',sizeof(users[fd].id));
      users[fd].playwith = -1;
      break;
    }
    
    message_handler(mesg,fd);
    bzero(mesg,MAXSIZE);
  }

  close(fd);
}

int main()
{
  int listenfd, connectfd;
  struct sockaddr_in server;
  struct sockaddr_in client;
  int in_size = sizeof(struct sockaddr_in);
  int number = 0;
  int fd;
  //initial structure
  for(int i=0; i<100; i++){
    for(int j=0; j<100; j++) users[i].id[j] = '\0';
    users[i].playwith = -1;
  }

  if((listenfd = socket(AF_INET, SOCK_STREAM,0)) == -1){
    perror("Error: Creating socket failed.");
    exit(1);
  }

  int opt = SO_REUSEADDR;
  setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

  bzero(&server,sizeof(server));

  server.sin_family = AF_INET;
  server.sin_port = htons(PORT);
  server.sin_addr.s_addr = htons(INADDR_ANY);
  //bind
  if(bind(listenfd,(struct sockaddr*)&server, sizeof(struct sockaddr)) == -1){
    perror("Error:Blinding error.");
    exit(1);
  }

  if(listen(listenfd,BACKLOG) == -1){
    perror("Error:Listening error.");
    exit(1);
  }
  printf("Waiting for client connecting....\n");

  while(1){
    if((fd = accept(listenfd,(struct sockaddr *)&client, &in_size)) == -1){
      perror("Error:Accepting error.");
      exit(1);
    }
    else printf("A client have connect.");
    if(number>=Max){
      printf("No more client can connect.\n");
      close(fd);
    }
    for(int i=0; i<Max; i++){
      if(fdt[i] == 0){
        fdt[i] = fd;
        break;
      }
    }
    //create new thread for new client
    pthread_t tid;
    pthread_create(&tid,NULL,(void*)pthread_service,&fd);
    number++;
  }
  close(listenfd);
}
