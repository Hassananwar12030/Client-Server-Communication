
#include <stdbool.h> // for boolean
#include <unistd.h> // for read, write, exec & fork.
#include <error.h>  // for error
#include <errno.h>  // to set error number
#include <string.h> // for string opt like strcpy
#include <stdlib.h> // for exit (_exit) (and atoi)
#include <signal.h>  // for kill API (SIGKILL & SIGCHLD) & signal API
#include <stdio.h>   //for Input Output
#include <pthread.h> //for threads

//For socket
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>



void *thread_func1(void* ptr);
void *thread_func2(void* ptr);
int sock;
int main(int argc, char *argv[])
	{
	
	struct sockaddr_in server;
	struct hostent *hp;
	char buf[1024];

	/* Create socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("opening stream socket");
		exit(1);
	}
	/* Connect socket using name specified by command line. */
	server.sin_family = AF_INET;
	hp = gethostbyname(argv[1]);
	if (hp == 0) {
		fprintf(stderr, "%s: unknown host\n", argv[1]);
		exit(2);
	}
	bcopy(hp->h_addr, &server.sin_addr, hp->h_length);
	server.sin_port = htons(atoi(argv[2]));

	if (connect(sock,(struct sockaddr *) &server,sizeof(server)) < 0) {
		perror("connecting stream socket");
		exit(1);
	}

	char xyz[]="The commands are: run, add, sub, mult, div, kill, list & listall\n\n";
    write(STDOUT_FILENO,xyz, strlen(xyz));


	pthread_t thread1;
	pthread_t thread2;

	int ret1 = pthread_create(&thread1,NULL,thread_func1,(void*) NULL);
	int ret2 = pthread_create(&thread2,NULL,thread_func2,(void*) NULL);
	if(ret1!=0 || ret2!=0){
		char x[]="Thread creation error\n";
		write(STDIN_FILENO,x,strlen(x));
		exit(1);
	}
	pthread_exit(NULL);
	/*ret1 = pthread_join(thread1,NULL);
	if(ret1!=0){
		char x[]="Thread join error\n";
		write(STDIN_FILENO,x,strlen(x));
		exit(1);
	}
	ret2 = pthread_join(thread2,NULL);
	if(ret2!=0){
		char x[]="Thread join error\n";
		write(STDIN_FILENO,x,strlen(x));
		exit(1);
	}*/
		
	close(sock);
}

void *thread_func1(void* ptr){
	int cntw = write(STDOUT_FILENO,"Enter the command with arguments here. For exit -1\n\n", 52);
	if (cntw == -1){
		perror("write on screen failed");
		exit(1);
	}
	while(1){
		char str[100]; // command in this buffer


		int cntRead = read(STDIN_FILENO, str, 100); // reading from terminal to str buff
		if (cntRead == -1){
			perror("reading from screen failed");
			exit(1);
		}

		str[cntRead-1]=NULL; // remove "enter" from the command
		if(strcmp(str,"-1")==0){ // if command is -1 then exit the client
			exit(0);
		}

		cntw = write(sock,str,cntRead); // writing on the socket to server to process command
		if (cntw == -1){
			perror("Couldn't write to socket");
			exit(1);
		}

	}
}

void *thread_func2(void* ptr){
	//sleep(10);
	bool check = true;
	while(1){
		char strOut[100];

		int cntRead = read(sock,strOut,100);// reading from server, when server writes to it
		if(cntRead!=100)
			strOut[cntRead]=NULL;

		char cpyOut[100];
		strcpy(cpyOut,strOut);
		char* token = strtok(cpyOut, " ");

		if(cntRead==-1){
			perror("Read from socket failed");
			exit(1);
		}

		else if(cntRead==0){
			write(STDOUT_FILENO,"Server is closed\n",17);
			exit(1);
		}

		else
		{ // writing output to the terminal
			char buff[100];
			int pointer=0;
			int i=0;
			while(pointer+i<cntRead){
				i++;
				if(strOut[pointer+i-1]==';'){
					if(check){
						write(STDOUT_FILENO,"OUTPUT: ", 8);
					}
					write(STDOUT_FILENO,buff,i-1);
					write(STDOUT_FILENO,"\n\n",2);
					pointer+=i;
					i=0;
					char buff[100];
					check=true;

				}
				else if(pointer+i==cntRead){
					if(check){
						write(STDOUT_FILENO,"OUTPUT: ", 8);
						//write(STDOUT_FILENO,"\n\n",2);
					}
					write(STDOUT_FILENO,buff,i);
					check=false;
					break;
				}
				else{
					buff[i-1]=strOut[pointer+i-1];
				}


			}

		}
	}

}

/*
else if(strcmp(token,"Msg_from_server:")==0){
	check = false;
	token = strtok(NULL, ";");
	char wrt[strlen(token)+25];
	int ret = sprintf(wrt,"Msg from server: %s\n\n",token);
	write(STDOUT_FILENO,wrt,ret);
}
*/