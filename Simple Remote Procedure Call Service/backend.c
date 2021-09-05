#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/mman.h>

#include "a1_lib.h"
#include "RPCServ.h"
#include "backend.h"

#define BUFSIZE 50

int main(int argc, char** argv){
    // check the input argument from command line
    if(argc != 3){
        printf("Number of argument not correct...\n");
        return 0;
    }

    // initialize the RPC server
    int server_fd;
    if(RPC_Init(argv[1], atoi(argv[2]), &server_fd)!=0){
        printf("Server cannot be initialized...\n");
        return 0;
    }

    //  show that server is listening
    printf("Server listening on %s:%s\n", argv[1], argv[2]);
    fflush(stdout);

    // client_fd for the frontend
    int client_fd; 

    // quit flag for the server, initialized to 0, if 'quit' or 'shutdown' is received, change the flag to 1
    int* quit_flag =(int*)mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0); 
    *quit_flag = 0;  

    // to store child pid when fork(), child would see 0, parent shoudl see child pid (not equal to 0) 
    int child_pid;

    // used by parent to track number of child remaining
    int child_num;

    // used to store return value of child
    int r_value;
    
    fflush(stdout);

    //waiting for the first connection from frontend 
    RPC_Accept_Connection(server_fd,&client_fd);
    //update child number, and show number of remaining child
    child_num = 1;
    printf("Number of child: %d\n", child_num);
    
    while(1){
        child_pid = fork();
        if(child_pid==0){                   // child process, to serve frontend
            int child_cfd = client_fd;      // pass client_fd to child
            while(1){
                message from_client = receive_cmd(client_fd);                   // receive, parse, and convert it to message struct format
                char* back_message = (char*) malloc (sizeof(char)*BUFSIZE);     // malloc memory to store message to send back
                find_command(from_client, back_message);                        // find and call command, and save it to back message
                if(*quit_flag == 1){                                            // if quit flag = 1, forcely change the message to quit message
                    strcpy(back_message,"Server is no longer available...");
                }
                send_message(child_cfd, back_message, strlen(back_message));    // send message back to frontend
                free(back_message);         // free malloced memory
                if(strcmp(from_client.command, "quit")==0||strcmp(from_client.command, "shutdown")==0){     // exit with 4 if quit or shutdown
                    exit(4);
                }else if(strcmp(from_client.command,"exit")==0||*quit_flag==1){ // exit with 1 if exit
                    exit(1);
                }
                free(from_client.command);  //free malloced memory
            }
        }else{              // parent process
            int child_counter = child_num;
            for (int i = 0; i < child_counter; i++){ // check return from all children, in case of multiple return before a new connection
                r_value = 0;
                waitpid(-1, &r_value, WNOHANG);
                if(WEXITSTATUS(r_value)==1){        // 1 indicate exit, only exit frontend, but not backend
                    child_num--;                       
                    printf("A child just quit, num left: %d\n", child_num);
                }else if(WEXITSTATUS(r_value)==4){  // 4 indicate quit and shutdown
                    *quit_flag = 1;                 // change quit flag to 1, indicate server is shuting down
                    child_num--;
                    break;                          // quit return is received, stop waiting for return of other children
                }
            }
                           
            if(*quit_flag == 1){                    // shut down process initialized
                shutdown(server_fd, 0);             // close the socket for new connection
                printf("A child just quit, num left: %d \nthe server is shutting down...\n", child_num);
                while(child_num != 0){              // wait for child to return, until all children return
                    waitpid(-1, NULL, 0);
                    child_num--;
                    printf("Number of child left: %d\n", child_num);
                }
                return 0;                           // close program after all children return
            }
            if(*quit_flag!=1){                      
                RPC_Accept_Connection(server_fd,&client_fd);    //keep accepting new connection if quit flag is not 1
            }
            child_num++;                            // update child number after receiving new connection
            printf("Number of child: %d\n", child_num);
        }
    }
}

// Initialize server
int RPC_Init(char* host, uint16_t port, int* sockfd){
    return create_server(host, port, sockfd);
}

// Accept Connection
int RPC_Accept_Connection (int server_sockfd, int* clientfd){
    if(accept_connection(server_sockfd,clientfd)==0){
        return 0;
    }
    return -1;
}

// reveive, parse and change to message struct format
message receive_cmd(int clientfd){
    message converted;
    converted.argument[0] = 0;
    converted.argument[1] = 0;
    converted.command = (char*)malloc(sizeof(char)*50);
    char buf[BUFSIZE];
    recv(clientfd, buf, BUFSIZE,0);
    char* real_message = strtok(buf, "\n");     //use  tokenizer to eliminate \n
    char* token = strtok(real_message, " ");    // use tokenizer to seperate command and parameters
    if(token == NULL){
        strcpy(converted.command,"Empty Command");
    }else{
        strcpy(converted.command,token);
    }
    int i = 0;
    while (1){      // count the number of parameter
        token = strtok(NULL, " ");
        if (token == NULL||i>1){
            break;
        }
        converted.argument[i] = atof(token);    //convert parameter to floats
        i++;
    }
    converted.component_num = i;    //record number of parameter
    return converted;
}

int addInts(int a, int b){
    return a+b;
}

int multiplyInts(int a, int b){
    return a*b;
}

float divideFloats(float a, float b){
    return a/b;
}

int server_sleep(float x){
    sleep(x);
    return 0;
}

uint64_t factorial(int x){
    uint64_t output = 1;
    while (x != 0){
        output = output*x;
        x--;
    }
    return output;
}

// find the command and put the result to the desired memory space
void find_command(message input, char* out){
    if(strcmp(input.command, "add")==0){        //add command
        if(input.component_num != 2){
            strcpy(out, "Error: Number of argument not correct...");
        }else{
            sprintf(out,"%d",addInts((int)input.argument[0], (int)input.argument[1]));
        }
    }else if(strcmp(input.command, "multiply")==0){     //multiply command
        if(input.component_num != 2){
            strcpy(out, "Error: Number of argument not correct...");
        }else{
            sprintf(out,"%d",multiplyInts((int)input.argument[0], (int)input.argument[1]));
        }
    }else if(strcmp(input.command, "divide")==0){       //float divide command
        if(input.component_num != 2){
            strcpy(out, "Error: Number of argument not correct...");
        }else if(input.argument[1]==0){
            strcpy(out, "Error: Division by Zero...");      //division by zero error
        }else{
            sprintf(out,"%.6f",(float)divideFloats(input.argument[0], input.argument[1]));
        }
    }else if(strcmp(input.command, "factorial")==0){        // factorial command
        if(input.component_num != 1){
            strcpy(out, "Error: Number of argument not correct...");
        }else if(input.argument[0]>20||input.argument[0]<0){
            strcpy(out, "Error: Input is not between 0 and 20...");     // parameter not within range error
        }else{
            sprintf(out,"%ld",factorial((int)input.argument[0]));
        }
    }else if(strcmp(input.command, "sleep")==0){
        if(input.component_num != 1){
            strcpy(out, "Error: Number of argument not correct...");
        }else if(input.argument[0]<0){
            strcpy(out, "Error: Input is smaller than 0...");       // sleep time less than 0 error
        }else{
            server_sleep(input.argument[0]);
            strcpy(out, " ");
        }
    }else if(strcmp(input.command, "quit")==0||strcmp(input.command, "shutdown")==0){
            strcpy(out, "Bye...");
    }else if(strcmp(input.command,"exit")==0){
            strcpy(out, "Bye...");
    }else{
        strcpy(out, "Error: Command \"");           // command not found
        strcat(out,input.command);
        strcat(out, "\" not found");
    }
}

