#ifndef BACK_H
#define BACK_H

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

/**
 * A struct to store information received from client
 * 
 * @params:
 *  command:    first token of the user input, should be command to be called
 *  argument:   second and third token of the user input, should be parameter of commands
 *  component_num:  number of arguments, should be between 0 and 2
 */
typedef struct message{
    char* command;
    float argument[2];
    int component_num;
}message;

/**
 * Add two integers together
 * 
 * @params:
 *   a:     First integer
 *   b:     Second integer
 * @return: an int with value of a+b
 */
int addInts(int a, int b);

/**
 * Multiply two integer together
 * 
 * @params:
 *   a:     First integer
 *   b:     Second integer
 * @return: an int with value of a*b
 */
int multiplyInts(int a, int b);

/**
 * Divide two floats
 * 
 * @params:
 *   a:     First integer
 *   b:     Second integer
 * @return: a float with value of a/b
 */
float divideFloats(float a, float b);

/**
 * Make process sleep for a certain period time
 * 
 * @params:
 *   x:     time interval to sleep, integer greater than 0
 * 
 * @return: 0 if sleep success 
 */
int server_sleep(float x);

/**
 * Factorial of a number
 * 
 * @params:
 *   x:     Integer to be computed for factorial (0 <= x <= 20)
 * 
 * @return: an int with value of a+b
 */
uint64_t factorial(int x);

/**
 * receive message from client, and convert it into a self-designed 
 * message struct using string tokenizing. 
 * 
 * @params:
 *   clientfd: client socket to receive message     
 *   
 * @return: a message struct after tokenizing the received message
 */
message receive_cmd(int clientfd);

/**
 * Find and call the coresponding command received from user input
 * 
 * @params:
 *   input:   message struct with all the command and parameters from the client    
 *   output:  position to store output of commands
 * 
 * @return:   no return
 */
void find_command(message input, char* output);

#endif