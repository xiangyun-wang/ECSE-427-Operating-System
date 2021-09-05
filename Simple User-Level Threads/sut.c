#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <ucontext.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "sut.h"
#include "queue.h"

#define DES_BUFSIZE 50          // buf size for ip address
#define READ_BUFSIZE 1024       // buf size for read
#define STACK_SIZE 16 * 1024    // stack size for ucontext

/**
 * message struct for IO queue (to_ithread and from_ithread)
 **/
typedef struct iomessage{
    int command;    // 1 for open, 2 for write, 3 for read, 4 for close
    char* selement; // string element
    int ielement;   // integer element
}iomessage;

void connection_initialize();   // initialization function for tcp connection
void connection_shutdown();     // close function for tcp connection
void clean_up();                // clean up function for shutdown 

struct queue c_queue;           // ready queue
struct queue wait_queue;        // wait queue
struct queue to_ithread;        // to_IO queue (send message to ithread)
struct queue from_ithread;      // from_IO queue (read from ithread)

pthread_t c_thread_handle;      // C thread handler
pthread_t i_thread_handle;      // I thread handler

ucontext_t parent;              // parent task
ucontext_t template;            // task template for new tasks

pthread_mutex_t c_lock = PTHREAD_MUTEX_INITIALIZER;     //mutex lock

int sockfd;         // sock number
char* host;         // ip host
int port;           // port number

char* read_from_io; // to save content popped from from_IO queue

struct queue_entry* next = NULL;    // next task from the ready queue

bool shutdown_flag = false;         // indicate whether all tasks are created
bool shutdown_c_flag = false;       // indicate whether all tasks are cleared from ready and wait queue

void* cthread(){
    getcontext(&template);                  // get a template ucontext for future new task
    while(true){
        if(next!=NULL){                     // memory management
            free(next);
        }
        pthread_mutex_lock(&c_lock);
        next = queue_pop_head(&c_queue);    // next task from ready queue
        pthread_mutex_unlock(&c_lock);
        if(next != NULL){
            swapcontext(&parent,(ucontext_t*)next->data);       //save status to parent and switch to next ready task
        }else{
            if(shutdown_flag){              // if all tasks created
                void* check_c;
                void* check_wait;
                pthread_mutex_lock(&c_lock);
                check_c = queue_peek_front(&c_queue);           // check ready queue
                check_wait = queue_peek_front(&wait_queue);     // check wait queue
                pthread_mutex_unlock(&c_lock);
                if(check_c == NULL && check_wait ==NULL){       // if both ready and wait queue are NULL
                    shutdown_c_flag = true;                     // thread shutdown flag = true
                    pthread_exit(NULL);                         // exit thread
                }
                usleep(100);                // sleep for 100 us
            }else{
                usleep(100);                // sleep for 100 us
            }
        }
    }
}

void* ithread(){
    struct queue_entry* next_io = NULL;           // use to save next io task
    int command_opt;                              // command option: 1 for open, 2 for write, 3 for read, 4 for close 
    while(true){
        pthread_mutex_lock(&c_lock);
        next_io = queue_pop_head(&to_ithread);    // next io task
        pthread_mutex_unlock(&c_lock);
        if (next_io != NULL){                     // if next io task not NULL
            command_opt = ((iomessage*)(next_io->data))->command;         // get io task
            if(command_opt == 1){                 // open task
                connection_initialize();          // initialize tcp connection
                pthread_mutex_lock(&c_lock);
                queue_insert_tail(&c_queue, queue_pop_head(&wait_queue)); // push the task from wait queue back to ready queue
                pthread_mutex_unlock(&c_lock);    
            }else if(command_opt == 2){           // write task
                send(sockfd, ((iomessage*)(next_io->data))->selement, ((iomessage*)(next_io->data))->ielement, 0);  // send message to destination
                free(((iomessage*)(next_io->data))->selement);            // memory management
            }else if(command_opt == 3){           // read task
                int flag = -1;                    // flag to indicate whether the message from the server is reveived successfully
                char* from_io = (char*)malloc(READ_BUFSIZE);              // malloc memory to receive data
                memset(from_io, 0, sizeof(READ_BUFSIZE));
                while(flag<0){                    // if flag < 0, keep receiving
                    flag = recv(sockfd, from_io, READ_BUFSIZE, 0);
                }
                pthread_mutex_lock(&c_lock);
                queue_insert_tail(&from_ithread, queue_new_node(from_io));// push the read data to from_io queue
                queue_insert_tail(&c_queue, queue_pop_head(&wait_queue)); // move the task from wait queue to ready queue
                pthread_mutex_unlock(&c_lock);
            }else if (command_opt == 4){          // close task
                connection_shutdown();            // close tcp connection
            }
            free((next_io->data));                // free iomessage struct
            free(next_io);                        // free next_io node
        }else{
            if(shutdown_flag&&shutdown_c_flag){   // if no tasks in ready and wait queue
                clean_up();                       // clean up memory
                pthread_exit(NULL);               // exit thread
            }else{
                usleep(100);                      // wait for 100 us
            }
        }
    }
}

void sut_init(){
    // create and init four queues
    c_queue = queue_create();           // ready queue
    queue_init(&c_queue);
    wait_queue = queue_create();        // wait queue
    queue_init(&wait_queue);
    to_ithread = queue_create();        // to_IO queue
    queue_init(&to_ithread);
    from_ithread = queue_create();      // from_IO queue
    queue_init(&from_ithread);

    host = (char*)malloc(DES_BUFSIZE);  // malloc memory for host ip address

    read_from_io = (char*)malloc(READ_BUFSIZE); // malloc for read from IO data

    // create 2 kernel level threads
    pthread_create(&c_thread_handle, NULL, cthread, &c_lock);   // C-EXEC
    pthread_create(&i_thread_handle, NULL, ithread, &c_lock);   // I-EXEC
}

bool sut_create(sut_task_f fn){
    if(fn == NULL){       // if function pointer is NULL, return false
        return false;
    }
    ucontext_t* new = (ucontext_t*)malloc(sizeof(ucontext_t));
    char* new_stack = (char*)malloc(STACK_SIZE);
    memcpy(new,&template,sizeof(ucontext_t));
    new->uc_stack.ss_sp = new_stack;            // new stack pointer
    new->uc_stack.ss_size = sizeof(new_stack);  // size of stack
    new->uc_link = &parent;                     // link back to parent if task is done
    makecontext(new, fn, 0);
    pthread_mutex_lock(&c_lock);
    queue_insert_tail(&c_queue, queue_new_node(new));   // push to ready queue
    pthread_mutex_unlock(&c_lock);
    return true;
}

void sut_shutdown(){
    shutdown_flag = true;                   // indicate all tasks are created
    pthread_join(c_thread_handle,NULL);     // wait for C-EXEC to complete
    pthread_join(i_thread_handle,NULL);     // wait for I-EXEC to complete
}

void sut_yield(){
    ucontext_t *cur_task = (ucontext_t*)malloc(sizeof(ucontext_t));     // create a new ucontext
    struct queue_entry *back = queue_new_node(cur_task);                // create a new node for ready queue
    pthread_mutex_lock(&c_lock);
    queue_insert_tail(&c_queue, back);      // push back to ready queue
    pthread_mutex_unlock(&c_lock);
    swapcontext(cur_task,&parent);          // save current task status and swap with parent task
}

void sut_exit(){
    free(((ucontext_t*)(next->data))->uc_stack.ss_sp);  // free stack
    free((ucontext_t*)(next->data));                    // free ucontext_t
    setcontext(&parent);        // back to parent task
}

void sut_open(char* destination, int port_num){
    strcpy(host,destination);           // copy destination address to global variable host
    port = port_num;                    // copy port number
    iomessage* message_to_io = (iomessage*)malloc(sizeof(iomessage));   // create an iomessage
    message_to_io->command = 1;         // set command to open
    struct queue_entry *to_io = queue_new_node(message_to_io);
    ucontext_t *cur_task = (ucontext_t*)malloc(sizeof(ucontext_t));     
    struct queue_entry *back = queue_new_node(cur_task);
    //lock
    pthread_mutex_lock(&c_lock);
    queue_insert_tail(&to_ithread, to_io);      // push iomessage to to_io queue
    queue_insert_tail(&wait_queue, back);       // push the task to wait queue
    //unlock
    pthread_mutex_unlock(&c_lock);
    swapcontext(cur_task,&parent);              // back to parent task
}

char* sut_read(){
    iomessage* message_to_io = (iomessage*)malloc(sizeof(iomessage));
    message_to_io->command = 3;     //set command to read
    struct queue_entry *to_io = queue_new_node(message_to_io);
    ucontext_t *cur_task = (ucontext_t*)malloc(sizeof(ucontext_t));
    struct queue_entry *back = queue_new_node(cur_task);
    // lock
    pthread_mutex_lock(&c_lock);
    queue_insert_tail(&to_ithread, to_io);  // push iomessage to to_io queue
    queue_insert_tail(&wait_queue, back);   // push task to wait queue
    pthread_mutex_unlock(&c_lock);
    swapcontext(cur_task,&parent);          // back to parent task
    
    // when data is ready
    pthread_mutex_lock(&c_lock);
    struct queue_entry* data_from_io = queue_pop_head(&from_ithread);   // pop data out
    pthread_mutex_unlock(&c_lock);
    memset(read_from_io,0,READ_BUFSIZE);
    strcpy(read_from_io,data_from_io->data);  // copy the data to read_from_io
    free(data_from_io->data);                 // memory management
    free(data_from_io);
    return read_from_io;                      // return read data
}

void sut_write(char* buf, int size){        
    char* selement = (char*)malloc(size);     
    strcpy(selement,buf);
    iomessage* message_to_io = (iomessage*)malloc(sizeof(iomessage));   
    message_to_io->command = 2;             // set command to write
    message_to_io->ielement = size;         // set write size
    message_to_io->selement = selement;     // set write string
    struct queue_entry *to_io = queue_new_node(message_to_io);
    pthread_mutex_lock(&c_lock);
    queue_insert_tail(&to_ithread, to_io);  // push to to_io queue
    pthread_mutex_unlock(&c_lock);
}

void sut_close(){
    //close the socket
    iomessage* message_to_io = (iomessage*)malloc(sizeof(iomessage));
    message_to_io->command = 4;             // set command to close
    struct queue_entry *to_io = queue_new_node(message_to_io);
    pthread_mutex_lock(&c_lock);
    queue_insert_tail(&to_ithread, to_io);  // push to to_io queue
    pthread_mutex_unlock(&c_lock);
    
}

void connection_initialize(){
    struct sockaddr_in server_address = { 0 };
    // create a new socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      perror("Failed to create a new socket\n");
    }
    // connect to server
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET, host, &(server_address.sin_addr.s_addr));
    server_address.sin_port = htons(port);
    if (connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
      perror("Failed to connect to server\n");
    }
}

void connection_shutdown(){
    shutdown(sockfd, SHUT_WR);  // shutdown write
    shutdown(sockfd, SHUT_RD);  // shutdown read
    close(sockfd);              // close socket
}

void clean_up(){
    free(host);                 // free host string
}
