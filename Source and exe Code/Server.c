#include <stdbool.h> // for boolean
#include <fcntl.h> // for pipe2
#include <time.h>   // for time
#include <unistd.h> // for read, write, exec & fork.
#include <error.h>  // for error
#include <errno.h>  // to set error number
#include <string.h> // for string opt like strcpy
#include <math.h>    // for maths opt like power 
#include <stdlib.h> // for exit (_exit) (and atoi)
#include <signal.h>  // for kill API (SIGKILL & SIGCHLD) & signal API
#include <sys/types.h> // for wait and socket
#include <sys/wait.h> // for waitpid
#include <stdio.h>  //for input, output
#include <pthread.h>

//for socket
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void *thread_read_usr(void *ptr); //this is for server reading from user and writing to pipe of certain CH
void *thread_CH_read_pipe(void *ptr);// this for client handler read command from pipe and writing the output back to main server(conn)
void *thread_Conn_read_pipe(void* ptr);// this for conn reading the output from pipe and writing to screen of server

struct Process{ 

    int pid; 
    char pname[20]; 

    time_t startTime; 
	time_t endTime; 
};

struct Process processes[100]; 



int counter = 0; // counter for processes array

int output; // for math output
int argNum; // for argument number (could also be used in operation function, that's why global)

int status; //to get the status of killed process

//for child handler
int chId =0;

/*child handler info*/
struct chInfo{ 
	int id;
	int pid;
	int rwfd;
	int fd1[2]; //conn will write to this pipe fd and ch will read
	int fd2[2]; //vise versa
	bool isClosed;
};
struct chInfo cHandlers[200];

static void Conn_SignalHandler(int signo){
	/* if client handler exits, remove it from client handlers list */
	if(signo==SIGCHLD){
		int cPid = waitpid(-1,&status,NULL);
		if(cPid==-1){
			perror("waitPid: ");
		}
		else if(cPid!=0){
			for(int i=0; i<chId; i++){
				if(cPid == cHandlers[i].pid){
					cHandlers[i].isClosed=true;
				}
			}
		}
	}
	else if(signo==SIGPIPE){
		char msg[]="ERROR: SIGPIPE occured inside server\n. Client Handler is closed before data could be wriiten to its pipe or Socket is closed before writing data to it\n";
		write(STDOUT_FILENO,msg,strlen(msg));
	}
}

static void CH_SignalHandler(int signo){
	/* if kill exec process from outside, update it's endtime */
	if(signo==SIGCHLD){
		int cPid = waitpid(-1,&status,NULL);
		if(cPid==-1){
			perror("waitPid: ");
		}
		else if(cPid!=0){

			for(int i=0; i<counter; i++){
				if(processes[i].pid==cPid){

					//write(STDOUT_FILENO, "Found Pid\n",10);
					processes[i].endTime = time(0);
				}
			}
		
		}
	}
	else if(signo==SIGPIPE){
		char msg[]="ERROR: SIGPIPE occured inside Client Handler\n. Server is closed before data could be wriiten to its pipe or Socket is closed before writing data to it\n";
		write(STDOUT_FILENO,msg,strlen(msg));
	}

}

/*apply mathematical operation like for add sub etc. and set it
result to output. If not valid number(s), return false otherwise
true*/ 
bool Operation(char *p,int cntp,int opt)
{
    bool minusSign = false;
    int x =0;
    char num[cntp];
    int power = cntp;
    
    for(int i=0; i<cntp; i++){

        char a = *p;
        int digit = (int)a;
        digit= digit-48;
        
        //*printf("d is %d \n",digit);
        if ((digit<0) || (digit>9)){

            if(digit==-3 && i==0 && cntp>1){
                
                if (minusSign ==false)
                    minusSign = true;
                
                else
                    minusSign=false;
                
                power--;
            }

            else
                return false;
            
        }

        else{

            int result =(int) pow(10,power-1);
			
            power--;
            result = result*digit;
            
            x=x+result;
        }

        p++;
    }

    if(argNum==0){

        if (!minusSign)
        output=x;
        else
        output  = x*-1;
    }

    else if(argNum!=0 && opt==1){

        if(minusSign == false)
            output=output+x;
        
        else
            output=output-x;
            //*minusSign == false;
        
    }

    else if(argNum!=0 && opt==2){
        if(minusSign == false)
            output=output-x;
        
        else
            output=output+x;
            //*minusSign == false;
        
    }

    else if(argNum!=0 && opt==3){

        if(minusSign == false)
            output=output*x;
        
        else
            output=output*x*-1;
            //*minusSign == false;
        
    }

     else if(argNum!=0 && opt==4){

        if(x==0)
            return false;

        else{

            if(minusSign == false)
                output=output/x;
            
            else
                output=(output/x)*-1;
                //*minusSign == false;
            
        }
    }

    else
        return false;
    


    return true;
}

/*write the list of processes one by to the fd given by rwfd.
listAll if asked for all list otherwise only for active processes.*/
void printProcessList(bool listAll,int rwfd){ 
	int sno=1;   //SNO for processes
	//char arryOut[300 * counter];
	//int noChars=0;
    if (listAll){

		if(counter>0){
        	char x[]="\n--------List of All Processes---------\n";
			//strcat(arryOut,x);
			//noChars+=strlen(x);
			write(rwfd,x, strlen(x));
		}
        else{

			char y[]="no process in the list\n";
			write(rwfd,y, strlen(y));
			//strcat(arryOut,y);
			//noChars+=strlen(y);
		}

         for(int i=0; i<counter; i++){// for each process in array
 
            time_t startT = processes[i].startTime; //startT is process's start time
            time_t endT = processes[i].endTime;    //endT is process's end time
            
            char* dtBefore = ctime(&startT);    // converting start time of process from seconds to date and time
			char* dtAfter;       				// same as above with end Time of process
            
           
            if (endT == -1){ // if process is still running
                dtAfter = " - "; 
                endT=time(0); // end time is current time to display in elapse time
            }
			else{
				dtAfter = ctime(&endT);
			}

            float diff = endT-startT; // differnce of seconds between end time and start time
            
            int noOfMinutes = diff/60; // for number of minutes
            int hour = noOfMinutes/60; // for number of hours
            int minute = noOfMinutes%60; // to display minutes
            
            char pDetails [200];      // for process details
            int cntR = sprintf(
				pDetails,
				"%d) Pid: %d Name: %s Start at: %s End at: %s Elapse: %d hours %d mins. \n",
				sno,processes[i].pid,processes[i].pname,dtBefore,dtAfter,hour,minute
			);
            
            int wrtCnt = write(rwfd,pDetails,cntR);
			//strcat(arryOut,pDetails);
			//noChars+=cntR;
			//if(wrtCnt<0)
				//perror("Writing to fd while printing list"); 

			sno++;

        }
    }
    
    else{

        bool anyActive =  false;
        for(int i=0; i<counter; i++){

            time_t endT = processes[i].endTime;

            if (endT == -1){
				if(!anyActive){
					anyActive=true;
					char x[]="\n---------List of Active Processes---------\n";
					write(rwfd,x, strlen(x));
					//strcat(arryOut,x);
					//noChars+=strlen(x);
				}

                time_t startT = processes[i].startTime;
                time_t currentT = time(0); //current time in seconds
                
                char* dtBefore = ctime(&startT);
                
                float diff = currentT-startT;
                
				//for execution time
                int noOfMinutes = diff/60;
                int hour = noOfMinutes/60;
                int minute = noOfMinutes%60;
                
                char pDetails [200];
                int cntR = sprintf(
					pDetails,
					"%d) Pid: %d Name: %s Start at: %s Execution time: %d hours %d minutes.\n",
					sno,processes[i].pid,processes[i].pname,dtBefore,hour,minute
				);
                
                int wrtCnt = write(rwfd,pDetails,cntR);
				//strcat(arryOut,pDetails);
				//noChars+=cntR;
				//if(wrtCnt<0)
					//perror("Writing to fd while printing list");
                
				sno++;

            }
        }
		if(!anyActive){
			char y[]="no active process in the list\n";
			int wrtCnt = write(rwfd,y, strlen(y));
			//strcat(arryOut,y);
			//noChars+=strlen(y);
			//if(wrtCnt<0)
				//perror("Writing to fd while printing list");
		}

    }

	write(rwfd, ";",1);
	//strcat(arryOut,";");
	//write(rwfd,arryOut,noChars+1);
}

/* kill the exec process by name or pid*/
int killProcess(char* s){
	bool iskill=false;
    int idGiven;
	int retV =sscanf(s,"%d",&idGiven);
	if(retV==0)
		idGiven=-1;

    for(int i=0; i<counter; i++){
        
        int procid = processes[i].pid;
        
		if (idGiven==procid || strcmp(processes[i].pname,s)==0){

            if(processes[i].endTime!=-1){
                iskill=true;
            }

            else{
                int ret = kill(procid, SIGTERM);
                if(ret==-1){
                    perror("kill process error: ");
                    return -2;
                }
                else{
					iskill=false;
                    return 1;
                }
            }
        }
    }
	if(iskill){
		return 0;
	}

    return -1;

}

/*Establishes socket and then in loop, waits for accepting the 
connection, then forks and create struct for new child handler 
and add it to the list. The child process, which is for client 
handler, will read command of client from socket, process the 
command and write the output back to socket*/
int main(void)
{
	int sock, length;
	struct sockaddr_in server,client;
	int msgsock;
	char str[100];
	int readCnt;


	/* Create socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("opening socket");
		exit(1);
	}

	/* Name socket using wildcards */
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = 0;

	if (bind(sock, (struct sockaddr *) &server, sizeof(server))) {
		perror("binding socket");
		exit(1);
	}

	/* Find out assigned port number and print it out */
	length = sizeof(server);

	if (getsockname(sock, (struct sockaddr *) &server, (socklen_t*) &length)) {
		perror("getting socket name");
		exit(1);
	}

	char portmsg [100];
	int retCnt = sprintf(portmsg,"Socket has port #%d\n", ntohs(server.sin_port));
	int ret = write(STDOUT_FILENO,portmsg,retCnt);	

	pthread_t thread2;	
	int ret2 = pthread_create(&thread2,NULL,thread_read_usr,(void*) NULL);
	if(ret2!=0){
		char x[]="Thread 'read server from user' creation error error\n";
		write(STDIN_FILENO,x,strlen(x));
		exit(1);
	}

	ret2 = pthread_detach(thread2);
	if(ret2!=0){
		char x[]="Thread 'read server from user' detach error\n";
		write(STDIN_FILENO,x,strlen(x));
		exit(1);
	}

	if(signal(SIGCHLD,Conn_SignalHandler)==SIG_ERR){
		char x[]= "Connection sigchld cannot be handled\n";
		write(STDOUT_FILENO,x,strlen(x));
		exit(1);
	}
	if(signal(SIGPIPE,Conn_SignalHandler)==SIG_ERR){
		char x[]= "Connection sigpipe cannot be handled\n";
		write(STDOUT_FILENO,x,strlen(x));
		exit(1);
	}

	/* Start accepting connections */
	listen(sock, 5);
	do {
		int len = sizeof(struct sockaddr_in);
		int msgsock = accept(sock, (struct sockaddr *) &client, (socklen_t*) &len);

		if (msgsock == -1)
			perror("accept failed");

		else{

			int fd1[2];
			int fd2[2];
			int p1= pipe(fd1);
			int p2 = pipe(fd2);
			if (p1<0 || p2<0)
				perror("read or write pipe for server failed: ");
			chId++;
		    int pid = fork();
			if(pid<0){
				perror("Fork failed");
				chId--;
				exit(1);
			}
			else if(pid>0){
				char connMsg[100];
				int ret = sprintf(connMsg,"Established connection with client %d \n\n",chId);
				write(STDOUT_FILENO,connMsg,ret);
				close(fd1[0]); // closing reading end for conn
        		close(fd2[1]);	// closing writing end for conn 
				struct chInfo newCh;
				newCh.id = chId;
				newCh.rwfd = msgsock;
				newCh.pid = pid;
				newCh.fd1[0] = fd1[0];
				newCh.fd1[1] = fd1[1];
				newCh.fd2[0] = fd2[0];
				newCh.fd2[1] = fd2[1];
				newCh.isClosed = false;
				cHandlers[chId-1]=newCh;
			}

		    else{

				if(signal(SIGCHLD,CH_SignalHandler)==SIG_ERR){
					char x[]= "Client Handler Sigchld cannot be handled\n";
					write(STDOUT_FILENO,x,strlen(x));
					exit(1);
				}
				if(signal(SIGPIPE,CH_SignalHandler)==SIG_ERR){
					char x[]= "Client Handler sigpipe cannot be handled\n";
					write(STDOUT_FILENO,x,strlen(x));
					exit(1);
				}
				
				close(fd1[1]); // closing writing end for CH
        		close(fd2[0]); // closing reading end for CH
				int rwfd = msgsock;

				pthread_t thread;
				struct chInfo ch;
				ch.rwfd = rwfd;
				ch.id=chId;
				ch.fd1[0] = fd1[0];
				ch.fd1[1] = fd1[1];
				ch.fd2[0] = fd2[0];
				ch.fd2[1] = fd2[1];

				int ret = pthread_create(&thread,NULL,thread_CH_read_pipe,(void*) &ch);
				if(ret!=0){
					char x[]="Thread 'CH read from pipe' creation error error\n";
					write(STDIN_FILENO,x,strlen(x));
					exit(1);
				}

				ret = pthread_detach(thread);
				if(ret!=0){
					char x[]="Thread 'CH read from pipe' cannot detached\n";
					write(STDIN_FILENO,x,strlen(x));
					exit(1);
				}
				
		     do {

		        bzero(str, sizeof(str));

				output=0;
				argNum=-1;

				bool checkRun=false; // to check if run command
				bool wrtSock = false; // to check if already writen to Socket
				
				char ansSt[100]; // to write on pipe
				int cntSt = 0; // count for how many bytes to write on pipe

				readCnt = read(rwfd, str, 100);

				if (readCnt < 0){
		            if(errno==EINTR){
						readCnt = read(rwfd, str, 100);
					}
					else{
						perror("reading stream message");
						exit(1);
					}
					
				}
		        if (readCnt == 0){
					char endConn[100];
					int ret = sprintf(endConn,"Ending connection with client %d\n\n",chId);
		            write(STDOUT_FILENO,endConn,ret);

					for(int i=0; i<counter; i++){
						if(processes[i].endTime==-1){
							int k = kill(processes[i].pid,SIGKILL);
							if(k<0){
								perror("Kill error ");
							}
						}
					}

					exit(0);
				}

		        else{

					if(str[0]==NULL){

						char pWrt[]="Command not entered;"; 
						int retWrt = write(rwfd,pWrt,strlen(pWrt));
						if(retWrt<0){
							perror("Writing to msgsock: ");
						}

						wrtSock=true;

					}

					else{

					//copying data so that 1st can be used to cal number of args
						char strCopy[readCnt]; 
						strncpy(strCopy,str,readCnt);

						char *token = strtok(str," ");
						char * command = token; //first token is the command

						int cnt=0; // for number of arguments
						while(token!= NULL){
							token = strtok(NULL, " ");
							cnt++;
						}

						//populating array of char pointer of argumnets, which ends with Null.
						char *args[cnt];
						token = strtok(strCopy, " ");
						int x=0;

						while(token!=NULL){
							token = strtok(NULL, " ");
							args[x]=token;
							x++;
						}

						if(strcmp(command,"run")==0){
							if(args[0]==NULL){
								cntSt = sprintf(ansSt,"%s","Please provide process name;");
								int retWrt = write(rwfd,ansSt,cntSt);
								if(retWrt<0){
									perror("writing output to socket");
								}

								wrtSock=true;
							}
							else{

								checkRun=true;
								int checkpipe[2];
								pipe2(checkpipe,O_CLOEXEC);
								

								int cPid = fork();
								if(cPid==-1){
									perror("fork");
								}

								else if(cPid==0){

									close(checkpipe[0]);
				
									int ret = execvp(args[0],args);
									if(ret==-1){
										char pWrt[]="Invalid Argument(s) passed to create this process;"; 
										int retWrt = write(checkpipe[1],pWrt,strlen(pWrt));
										if(retWrt<0){
											perror("writing to exec pipe");
										}
										exit(1);

									}

								}
								else{

									char outBuff[100];
									close(checkpipe[1]);
									
									int s = read(checkpipe[0],outBuff,100);

									if(s==-1){
										if(errno==EINTR){
											s = read(checkpipe[0],outBuff,100);
										}
										else{
											perror("Read from pipe: ");
										}			

									}
									
									else if(s == 0){
										struct Process newProc;

										//initializing process with info
										newProc.pid = cPid;

										strcpy(newProc.pname,args[0]);
										newProc.startTime = time(0);
										newProc.endTime = -1;


										processes[counter]=newProc;
										
										counter++;

									}

									else if(s >0){
										
										int ret = write(rwfd,outBuff,strlen(outBuff));
										if(ret==-1)
											perror("write to socket");
										else
											wrtSock=true;
									}
								}
							}

						}


						else if(strcmp(command,"kill")==0){
							if(args[0]==NULL){
								cntSt = sprintf(ansSt,"%s","Please provide pid or name;");
							}

							else{

								int retKill = killProcess(args[0]);
							
								if(retKill==-1)
									cntSt = sprintf(ansSt,"%s","no process with this name or pid;");
								
								else if(retKill==1)
									cntSt = sprintf(ansSt,"%s","Killed sucessfully;");
								
								else if(retKill==0)
									cntSt = sprintf(ansSt,"%s","Process already killed;");
								
								else{
									perror("kill");
									cntSt = sprintf(ansSt,"%s","Kill API Error;");
								}
							}

							int retWrt = write(rwfd,ansSt,cntSt);
							if(retWrt<0){
								perror("writing output to socket");
							}

							wrtSock=true;

						}

						else if(strcmp(command,"list")==0){
							if(args[0]!=NULL){
								cntSt = sprintf(ansSt,"%s","Invalid Command, arguments shouldn't be passed here;");
								int retWrt = write(rwfd,ansSt,cntSt);
								if(retWrt<0){
									perror("writing output to socket ");
								}

								wrtSock=true;
							}
							else{
								printProcessList(false,rwfd);
								wrtSock=true;
							}

						}

						else if(strcmp(command,"listall")==0){
							
							if(args[0]!=NULL){
								cntSt = sprintf(ansSt,"%s","Invalid Command, arguments shouldn't be passed here;");
								write(rwfd,ansSt,cntSt);
								wrtSock=true;
							}
							else{
								printProcessList(true,rwfd);
								wrtSock=true;
							}

						}

						else{

							int optNum=0;
							if(strcmp(command,"add")==0)
								optNum=1;

							else if(strcmp(command,"sub")==0)
								optNum=2;
							
							else if(strcmp(command,"mult")==0)
								optNum=3;
							
							else if(strcmp(command,"div")==0)
								optNum=4;
							
							else{

								char pWrt[]="Invalid Command;"; 
								int retWrt = write(rwfd,pWrt,strlen(pWrt));
								if(retWrt<0){
									perror("writing output to socket ");
								}
								wrtSock=true;

							}

							if(optNum!=0){
								for(int k=0; k<cnt-1; k++){
									argNum++;
									bool validArg = Operation(args[k],strlen(args[k]),optNum);
								
									if (!validArg){

										char pWrt[]="Invalid Argument(s);"; 
										int retWrt = write(rwfd,pWrt,strlen(pWrt));
										if(retWrt<0){
											perror("writing output to socket ");
										}

										wrtSock=true;
										break;

									}
								}
							}


						}

					}
					
					if(!wrtSock){

						if(checkRun==true)
							cntSt = sprintf(ansSt,"%s","Process Run Sucessfully;");
						
						else
							cntSt = sprintf(ansSt, "%d;",output);
						

						int cntw=write(rwfd,ansSt, cntSt);
						if (cntw == -1){
							perror("writing to pipe");
							exit(1);
						}
					}

				}


		    } while (readCnt != 0);

		    	close(msgsock);

		    }
		}
	} while (1);

}

void printGuidelineForServer(){
	char guide[] = "\n------------------Guideline for commands------------------\n";
	write(STDOUT_FILENO,guide,strlen(guide));
	
	char msg[]="\nFirst word of command is the specifier and second is command itself.\n\nFor specific client handler specifier would be: \"ch<id>\".\nFor all client Handlers specifier would be: \"all\"\n\n";
	write(STDOUT_FILENO,msg,strlen(msg));
	
	char msgCom[]="And the commands are: print, list and listall\n\n";
	write(STDOUT_FILENO,msgCom,strlen(msgCom));

	char dots[]= "-------------------------Enter Commands-----------------------\n\n";
	write(STDOUT_FILENO,dots,strlen(dots));
}

/*Conn's, this will continuously read command from server. It also
checks for which client handler the command is and sends command
to corresponding child handler's pipe*/
void *thread_read_usr(void *ptr){
	printGuidelineForServer();
	
	while(1){
		write(STDOUT_FILENO,"\n",1);
		char str [100];
		int cnt = read(STDIN_FILENO, str, 100);
		if(cnt<0){
			if(errno==EINTR){
				cnt = read(STDIN_FILENO, str, 100);
			}
			else
				perror("Read from server terminal");
		}

		str[cnt-1]=NULL; // removing enter

		bool isInput=true;
		bool correctSpecifier = false;
		bool isMsgWrten = false;

		char copyStr[100];
		strcpy(copyStr,str);
		char *command = strtok(copyStr, " ");
		command = strtok(NULL, " "); //actual command, like list
		char * arg = strtok(NULL, " ");

		char *token = strtok(str, " ");
		if(token == NULL){
			write(STDOUT_FILENO, "Error: no command entered\n", strlen("Error: no command entered\n"));
			isMsgWrten=true;
			isInput=false;
		}
		else if(command!=NULL){
			if(strcmp(command,"print")==0){
				if(arg==NULL){
					write(STDOUT_FILENO,"Error: Please provide message as well\n",38);
					isMsgWrten=true;
				}
			}
			else if(strcmp(command,"list")==0 || strcmp(command,"listall")==0){
				if(arg!=NULL){
					write(STDOUT_FILENO,"Error: arguments shouldn't be given here\n",41);
					isMsgWrten=true;
				}
			}
			else{
				write(STDOUT_FILENO,"Error: Invalid command\n",23);
				isMsgWrten=true;
			}
			
		}
		else if(command==NULL){
			write(STDOUT_FILENO,"Error: No command entered\n",26);
			isMsgWrten=true;
		}
		
		if(!isMsgWrten){
			if(strcmp(token,"all")==0){
				correctSpecifier=true;
				bool checkActiveCH=false;

				token = strtok(NULL, "");
				for(int i = 0; i<chId; i++){
					if(!(cHandlers[i].isClosed)){
						checkActiveCH=true;
						int retWrt = write(cHandlers[i].fd1[1],token,strlen(token));
						if(retWrt<0)
							perror("writing command to client handler");

						pthread_t thread;
						int ret = pthread_create(&thread,NULL, thread_Conn_read_pipe, (void*) &cHandlers[i]);
					
						if(ret!=0){
							char x[]="Thread 'Conn read from pipe' creation error error\n";
							write(STDIN_FILENO,x,strlen(x));
							exit(1);
						}
						//write(STDOUT_FILENO,"thread kay baad\n",17);
						ret = pthread_detach(thread);
						if(ret!=0){
							char x[]="Thread 'Conn read from pipe' cannot detached\n";
							write(STDIN_FILENO,x,strlen(x));
							exit(1);
						}	

					}
				}

				if(!checkActiveCH){
					write(STDOUT_FILENO,"Error: no active client exists\n", strlen("Error: no active client exists\n"));
				}
			}
			else{
				char com[strlen(token)];
				strcpy(com,token);
				int id=0;
				//to check specifier
				if((com[0]=='c' || com[0]=='C') && (com[1]=='h' || com[1]=='H') && (strlen(token)>2)){
					correctSpecifier=true;
					int power = strlen(token)-2;
					for(int i=2; i<strlen(token); i++){
						int digit = (int) com[i];
						digit-=48;
						if((digit<0) || (digit>9)){
							correctSpecifier=false;
							break;
						}
						else{
							
							int result =(int) pow(10,power-1);
							power--;
							result = result*digit;			
							id+=result;

						}
					}
				}
				else{
					correctSpecifier=false;
				}
				
				if(correctSpecifier){

					if(id<=chId && id>0){
						if(!(cHandlers[id-1].isClosed)){
							token = strtok(NULL, "");
							int retWrt = write(cHandlers[id-1].fd1[1],token,strlen(token));
							if(retWrt<0) 
								perror("writing to client handler's pipe");

							pthread_t thread;
							
							int ret = pthread_create(&thread,NULL, thread_Conn_read_pipe, (void*) &cHandlers[id-1]);

							if(ret!=0){
								char x[]="Thread 'Conn read from pipe' creation error error\n";
								write(STDIN_FILENO,x,strlen(x));
								exit(1);
							}
							
							ret = pthread_detach(thread);
							if(ret!=0){
								char x[]="Thread 'Conn read from pipe' cannot detached\n";
								write(STDIN_FILENO,x,strlen(x));
								exit(1);
							}	

						}
						else{
							write(STDOUT_FILENO,"Error: Client Handler already closed!\n",strlen("Error: Client Handler already closed!\n"));

						}
						
					}
					else{
						write(STDOUT_FILENO,"Error: Client handler doesn't exist\n",strlen("Error: Client handler doesn't exist\n"));

					}
				}
			}

			if(isInput && !correctSpecifier){
				write(STDOUT_FILENO,"Error: Incorrect Specifier\n",strlen("Error: Incorrect Specifier\n"));
			}
		}

	}
}

/* this thread is of a child handler and it will continuously
read command command from pipe, process it and sends data to conn's pipe*/
void *thread_CH_read_pipe(void *ptr){
struct chInfo *ch = (struct chInfo *) ptr;

	while(1){
		struct chInfo *ch = (struct chInfo *) ptr;
		char str[100];
	
		int cnt = read(ch->fd1[0],str,100);
		if(cnt<0){
			if(errno==EINTR){
				cnt = read(ch->fd1[0],str,100);
			}
			else
				perror("CH read from pipe");
		}

		if(cnt!=100)
			str[cnt]=NULL;
		char* token = strtok(str, " ");

		if(strcmp(token,"print")==0){
			token = strtok(NULL, "");
			char buff[strlen(token)+30];
			int ret = sprintf(buff,"Msg from server: %s;",token);
			cnt = write(ch->rwfd,buff,ret); // writing to the socket directly
			
			if(cnt!=-1){
				char out[100];
				cnt = sprintf(out,"message written to CH%d successfully\n\n",ch->id);
				int retWrt = write(ch->fd2[1],out,cnt);
				if(retWrt<0)
					perror("client handler wtiting to conn's pipe");

			}
			else
				perror("CH writing pipe");
		}
		else if(strcmp(token,"list")==0){
			char out[100];
			
			cnt = sprintf(out,"list of CH%d: ",ch->id);
			write(ch->fd2[1],out,cnt);
			printProcessList(false,ch->fd2[1]);
		}
		else if(strcmp(token,"listall")==0){
			char out[100];

			cnt = sprintf(out,"listall of CH%d: ",ch->id);
			write(ch->fd2[1],out,cnt);
			printProcessList(true,ch->fd2[1]);
			
		}
		else{
			char out[100];
			cnt = sprintf(out,"Wrong command given to CH%d \n",ch->id);
			write(ch->fd2[1],out,cnt);
		}
	}
}

/* this thread is of conn and it will finally read output from
pipe to which child handler has given output. Then the output is
showed to screen */
void *thread_Conn_read_pipe(void* ptr){

	struct chInfo *ch = (struct chInfo*) ptr;
	char str[100];
	int cnt = read(ch->fd2[0],str,100);

	if(cnt<0){
		if(errno==EINTR){
			cnt = read(ch->fd2[0],str,100);
		}
		else
			perror("Conn read data from pipe");
	}
	else if(cnt==0){
		char out[100];
		int ret = sprintf(out,"Sorry, Client %d closed before writing data\n",ch->id);
		write(STDOUT_FILENO,out,ret);
	}
	else{
		
		write(STDOUT_FILENO,"\n",strlen("\n"));
		write(STDOUT_FILENO,str,cnt);
		while(cnt!=0){
			cnt=read(ch->fd2[0],str,100);
			if(cnt<0){
				if(errno==EINTR){
					cnt=read(ch->fd2[0],str,100);
				}
				else
					perror("Conn read data from pipe");
			}
			else if(cnt==100){
				write(STDOUT_FILENO,str,cnt);
			}
			else if(cnt==0){
				break;
			}
			else{
				write(STDOUT_FILENO,str,cnt-1);
			}
		}
	}
	write(STDOUT_FILENO, "\n\n\n",3);
}