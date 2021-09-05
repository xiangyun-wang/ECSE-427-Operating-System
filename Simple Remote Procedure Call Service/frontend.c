#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "RPCServ.h"
#include "a1_lib.h"

#define BUFSIZE 50

int main(int argc, char** argv){
    if (argc != 3){
        printf("Number of arguments for initialization is not corrent...\n");
        return 0;
    }

    int sockfd;
    char user_msg[BUFSIZE] = {0};
    char server_msg[BUFSIZE] = {0};

    if(connect_to_server(argv[1], atoi(argv[2]), &sockfd)!=0){
        printf("Cannot connect to server...\n");
        return 0;
    }

    while(strcmp(user_msg, "quit\n")!=0&&strcmp(user_msg, "shutdown\n")!=0&&strcmp(user_msg, "exit\n")!=0){
        memset(user_msg, 0, sizeof(user_msg));
        memset(server_msg, 0, sizeof(server_msg));
        printf(">> ");

        fgets(user_msg, BUFSIZE, stdin);
        
        send(sockfd, user_msg, strlen(user_msg),0);

        recv(sockfd, server_msg, sizeof(server_msg),0);

        printf("%s\n",server_msg);
        fflush(stdout);

        if(strcmp(server_msg,"Server is no longer available...")==0){
            break;
        }
    }
    return 0;
}

int RPC_Connect (char* host, uint16_t port, int* sockfd){
    return connect_to_server(host, port, sockfd);
}