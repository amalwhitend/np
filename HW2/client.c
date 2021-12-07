#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <string.h>

#define PORT 1234
#define MAXDATASIZE 100

char sendbuf[1024];
char recvbuf[1024];
char name[100];
int fd;
int board[9];

void menu(){
  printf("Welcome to tic-tac-toe game.\n");
  printf("Enter \"1\" to change your name.\n");
  printf("Enter \"2\" to list all active user.\n");
  printf("Enter \"3 (your name) (other's name)\" to invite the other player.\n");
  printf("Enter \"logout\" to logout.\n");
}

void clear_board(int *board){
  for(int i=0; i<9; i++)
    board[i] = 0;
}

void print_board(int *board){
  char board_ox[9][2] = {'\0'};
  for(int i=0; i<9; i++){
    if(board[i] == 1) board_ox[i][0] = 'O';
    else if(board[i] == 2) board_ox[i][0] = 'X';
    else board_ox[i][0] = ' ';
  }
  printf("┌───┬───┬───┐        ┌───┬───┬───┐\n");
  printf("│ 0 │ 1 │ 2 │        │ %c │ %c │ %c │\n", board_ox[0][0], board_ox[1][0], board_ox[2][0]);
  printf("├───┼───┼───┤        ├───┼───┼───┤\n");
  printf("│ 3 │ 4 │ 5 │        │ %c │ %c │ %c │\n", board_ox[3][0], board_ox[4][0], board_ox[5][0]);
  printf("├───┼───┼───┤        ├───┼───┼───┤\n");
  printf("│ 6 │ 7 │ 8 │        │ %c │ %c │ %c │\n", board_ox[6][0], board_ox[7][0], board_ox[8][0]);
  printf("└───┴───┴───┘        └───┴───┴───┘\n");
}

int choose_user_turn(int *board){
  int i=0;
  int inviter=0,invitee=0;
  for(int i=0; i<9; i++){
    if(board[i] == 1) inviter++;
    else if(board[i] == 2) invitee++;
  }
  if(inviter > invitee) return 2;
  else return 1;
 
}

void write_on_board(int *board, int location){
  if(board[location] != 0){
    printf("You can't fill here!\nEnter \"0\" ~ \"8\" to choose again..");
    return;
  }
  print_board(board);//?
  int user_choise = choose_user_turn(board);
  board[location] = user_choise;
  sprintf(sendbuf,"7 %d %d %d %d %d %d %d %d %d", board[0],board[1],board[2],board[3],board[4],board[5],board[6],board[7],board[8]); 
}

void pthread_recv(void* ptr){
  int instruction;
  while(1){
    memset(sendbuf,0,sizeof(sendbuf));
    instruction = 0;
    if((recv(fd,recvbuf,MAXDATASIZE,0)) == -1){
      printf("recv() error\n");
      exit(1);
    }
    //printf("%s",recvbuf);
    sscanf(recvbuf,"%d",&instruction);
  switch(instruction){
    //list
    case 2:{
      printf("%s\n",&recvbuf[2]);//behind instruction
      break;
    }
    case 4:{
      char inviter[100];
      sscanf(recvbuf,"%d %s", &instruction,inviter);
      printf("%s\n",&recvbuf[2]);//"(name) is invite you"
      printf("Enter \"5 1 %s\" if accept.\n",inviter);
      printf("Else, enter \"5 0 %s\".\n",inviter);
      break;
    }
    case 6:{
      printf("\n---------------------\n");
      printf("\nGAME START!!\n");
      printf("\n---------------------\n");

      printf("Inviter hold O.\n");
      printf("Invitee hold X.\n");

      printf("Inviter go first.");
      printf("Please enter \"@0\"~\"@8\" to selecting your chess.\n");
      clear_board(board);
      print_board(board);
      break;
    }
    case 8:{
      char msg[100];
      sscanf (recvbuf,"%d %d%d%d%d%d%d%d%d%d %s",&instruction, &board[0],&board[1],&board[2],&board[3],&board[4],&board[5],&board[6],&board[7],&board[8], msg);
      print_board(board);
      printf("%s\n", msg);
      printf("Please input: '@0' ~ '@8' \n");
      break;
    }
    default:
      break;
  }
  }
}

int main(int argc, char *argv[]){
  int num;
    char buf[MAXDATASIZE];
    struct hostent *he;
    struct sockaddr_in server;


    if (argc !=2){
        printf("Usage: %s <IP Address>\n",argv[0]);
        exit(1);
    }
    if ((he=gethostbyname(argv[1]))==NULL){
        printf("gethostbyname() error\n");
        exit(1);
    }

    if ((fd=socket(AF_INET, SOCK_STREAM, 0))==-1)
    {
        printf("socket() error\n");
        exit(1);
    }
    bzero(&server,sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr = *((struct in_addr *)he->h_addr);
    if(connect(fd, (struct sockaddr *)&server,sizeof(struct sockaddr))==-1)
    {
        printf("connect() error\n");
        exit(1);
    }

  //add user
  printf("connect success.\n");
  printf("Please enter your name:");
  fgets(name,sizeof(name),stdin);
  char package[100];
  strcat(package,"1 ");
  strcat(package,name);
  send(fd,package,(strlen(package)),0);
  //menu
  menu();
  //create thread
  pthread_t tid;
  pthread_create(&tid, NULL, (void*)pthread_recv,NULL);
  while(1){
    memset(sendbuf,0,sizeof(sendbuf));
    fgets(sendbuf,sizeof(sendbuf),stdin);
    int location;
    if(sendbuf[0] == '@'){
      sscanf(&sendbuf[1],"%d",&location);
      write_on_board(board,location);
    }
    send(fd,sendbuf,(strlen(sendbuf)),0);
    //logout
    if(strcmp(sendbuf,"logout\n") == 0){
      memset(sendbuf,0,sizeof(sendbuf));
      printf("Logout success.");
      return 0;
    }
  }
  pthread_join(tid,NULL);
  close(fd);
  return 0;
}
