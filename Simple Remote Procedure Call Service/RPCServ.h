#ifndef RPCSERV_H
#define RPCSERV_H

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "a1_lib.h"

/**
 * To initialize an RPC server
 * 
 * @params:
 *   host:    server host ip
 *   port:    port number on the server to listen
 *   sockfd:  socked number   
 * 
 * @return:     if server initialized susccessfully, return 0
 *              else return -1;
 */
int RPC_Init(char* host, uint16_t port, int* sockfd);

/**
 * To accept connection from a user frontend
 * 
 * @params:
 *    server:   server to accept connection
 *    clientfd: client socket that would be used to communicate
 * 
 * @return: successfully accept connection will return 0
 *          otherwise, return -1
 */
int RPC_Accept_Connection (int server, int* clientfd);

/**
 * To connect to a server
 * 
 * @params:
 *    host:     ip address that wants to connect to 
 *    port:     port of the ip address to conenct to
 *    sockfd:   server socket that would be used to communicate with server
 * 
 * @return:     succesfully connect to a server will return 0
 *              otherwise return -1
 */
int RPC_Connect (char* host, uint16_t port, int* sockfd);


#endif