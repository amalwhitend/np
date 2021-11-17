#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#define PORT "80" // 提供給使用者連線的 port
#define BACKLOG 10 // 有多少個特定的連線佇列（pending connections queue）
#define BUFSIZE 8096

struct {
	char *ext;
	char *filetype;
} extensions [] = {
	{"gif", "image/gif" },
	{"jpg", "image/jpeg"},
	{"jpeg","image/jpeg"},
	{"png", "image/png" },
	{"zip", "image/zip" },
	{"gz",  "image/gz"  },
	{"tar", "image/tar" },
	{"htm", "text/html" },
	{"html","text/html" },
	{"exe","text/plain" },
	{0,0} };

void load (char buffer[BUFSIZE+1],int fd,int ret)   {
    char *tmp = strstr (buffer,"filename");
    if (tmp == 0 ) return;
    char filename[BUFSIZE+1],data[BUFSIZE+1],location[BUFSIZE+1];
    memset (filename,'\0',BUFSIZE);
    memset (data,'\0',BUFSIZE);
    memset (location,'\0',BUFSIZE);


    char *a,*b;
    a = strstr(tmp,"\"");
    b = strstr(a+1,"\"");
    strncpy (filename,a+1,b-a-1);
    strcat (location,"upload/");
    strcat (location,filename);

    a = strstr(tmp,"\n");
    b = strstr(a+1,"\n");
    a = strstr(b+1,"\n");
    b = strstr(a+1,"---------------------------");

    int download = open(location,O_CREAT|O_WRONLY|O_TRUNC|O_SYNC,S_IRWXO|S_IRWXU|S_IRWXG);

    char t[BUFSIZE+1];
    int last_write,last_ret;
    if (b != 0)
    write(download,a+1,b-a-3);
    else {
    int start = (int )(a - &buffer[0])+1;
    last_write = write(download,a+1,ret -start -61);
    last_ret = ret;
    memcpy (t,a+1+last_write,61);

    while ((ret=read(fd, buffer,BUFSIZE))>0) {
        write(download,t,61);
        last_write = write(download,buffer,ret - 61);
        memcpy (t,buffer+last_write,61);
        last_ret = ret;
        if (ret!=8096)
            break;
    }
    }

    close(download);
    printf ("UPLOAD FILE NAME :%s\n",filename);
    return ;
}

void handle_socket(int fd)
{
	int gp_fl = 1; // 0 == GET, 1 == POST
	int j, file_fd, buflen, len;
	long i, ret;
	char * fstr;
	static char buffer[BUFSIZE+1];
	// memset(buffer, '\0', BUFSIZE+1); 
	static char tmp[BUFSIZE+1];

	ret = read(fd,buffer,BUFSIZE);   /* 讀取瀏覽器要求 */
	if (ret==0||ret==-1) {
		/* 網路連線有問題，所以結束行程 */
		exit(3);
	}
	printf(" -------o %s o--------- ", buffer);
	memcpy(tmp, buffer, ret);
	load(buffer, fd, ret);
	/* 程式技巧：在讀取到的字串結尾補空字元，方便後續程式判斷結尾 */
	if (ret>0 && ret<BUFSIZE)
		buffer[ret] = 0;
	else
		buffer[0] = 0;

	/* 移除換行字元 */
	for (i=0;i<ret;i++) 
		if (buffer[i]=='\r'||buffer[i]=='\n')
			buffer[i] = 0;

	/* 若為 GET 要求 */
	if (strncmp(buffer,"GET ",4) == 0 || strncmp(buffer,"get ",4) == 0)
	{
		gp_fl = 0;		
	}

	/* 若為 POST 要求 */
	else if (strncmp(tmp,"POST ",5) == 0 || strncmp(tmp,"post ",5) == 0){
		gp_fl = 1; // Meaning it is POST now.
	}

	else
	{
		exit(3);
	}

	/* 我們要把 GET /index.html HTTP/1.0 後面的 HTTP/1.0 用空字元隔開 */
	if(gp_fl == 0)
		for(i=4;i<BUFSIZE;i++) {
			if(buffer[i] == ' ') {
				buffer[i] = 0;
				break;
			}
		}

	/* (GET)當客戶端要求根目錄時讀取 index.html */
	if (!strncmp(&buffer[0],"GET /\0",6)||!strncmp(&buffer[0],"get /\0",6) )
	{
		strcpy(buffer,"GET /index.html\0");
	}

	/* 檢查客戶端所要求的檔案格式 */
	buflen = strlen(buffer);
	fstr = (char *)0;

	for(i=0;extensions[i].ext!=0;i++) {
		len = strlen(extensions[i].ext);
		if(!strncmp(&buffer[buflen-len], extensions[i].ext, len)) {
			fstr = extensions[i].filetype;
			break;
		}
	}

	/* 檔案格式不支援 */
	if(fstr == 0) {
		fstr = extensions[i-1].filetype;
	}

	file_fd=open(&buffer[5],O_RDONLY);

	//printf("----%s----", &buffer[5]);
	/* 開啟檔案 */
	/*if((file_fd=open(&buffer[5],O_RDONLY))==-1)
	  {	    
	  write(fd, "Failed to open file", 19);
	  }*/	
	
	/* POST redirect */
	if( (gp_fl == 1) )
	{
		file_fd = open("index.html", O_RDONLY);  // index.html
		sprintf(tmp,"HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n");
		write(fd,tmp,strlen(tmp));
	}
	else{
		/* 傳回瀏覽器成功碼 200 和內容的格式 */
		sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
		write(fd,buffer,strlen(buffer));
	}
	//write(fd,buffer,strlen(buffer));


	/* 讀取檔案內容輸出到客戶端瀏覽器 */
	while ((ret=read(file_fd, buffer, BUFSIZE))>0) {
		write(fd,buffer,ret);
	}

	exit(1);
}


void sigchld_handler(int s)
{
	while(waitpid(-1,NULL,WNOHANG)>0);
}


int main(void)
{

	int sockfd, new_fd; // 在 sock_fd 進行 listen，new_fd 是新的連線
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // 連線者的位址資訊 
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // 使用我的 IP
	hints.ai_protocol=0 ;

	getaddrinfo(NULL, PORT, &hints, &servinfo);

	sockfd = socket(servinfo->ai_family, servinfo->ai_socktype,servinfo->ai_protocol);

	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int));

	if(bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen)==-1)
	{
		perror("bind");
		exit(1);
	}	  

	freeaddrinfo(servinfo);

	listen(sockfd, BACKLOG);


	sa.sa_handler = sigchld_handler; // 收拾全部死掉的 processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	sigaction(SIGCHLD, &sa, NULL);


	printf("server: waiting for connections...\n");


	while(1) // 主要的 accept() 迴圈
	{	  
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) 
		{
			perror("accept");
			continue;
		}


		if (!fork()) // 這個是 child process
		{

			close(sockfd); // child 不需要 listener
			handle_socket(new_fd);
			close(new_fd);

			exit(0);
		}

		close(new_fd); // parent 不需要這個
	}


	return 0 ;

}

