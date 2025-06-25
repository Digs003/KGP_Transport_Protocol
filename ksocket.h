/*===========================================*/
//Assignment 3
//Name-Diganta Mandal
//Roll-22CS30062
/*============================================*/
#ifndef KSOCKET_H
#define KSOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>


#define T 5
#define p 0.05
#define SOCK_KTP 3
#define N 10
#define P(s) semop(s, &pop, 1)
#define V(s) semop(s, &vop, 1)
#define MAX_SEQ_NUM 256

//Custom errornames
#define ENOTBOUND 200
#define ENOSPACE 201
#define ENOMESSAGE 202


typedef struct net_socket{
    int sock_id;                     // Stores the UDP socket ID
    char ip_addr[INET_ADDRSTRLEN];  // Stores the IP address
    uint16_t port;                 // Stores port number
    int err_code;                 // Global error variable
} NET_SOCKET;

typedef struct window{
    int wnd[256];           // For swnd: i=ith message in send buffer sent but not acknowledged, -1=otherwise
                            // For rwnd: i=expecting ith message in receiver buffer, -1=otherwise
    int size;               // Size of window
    int start;              // Start sequence number of window
} window;

typedef struct Shared_Mem{
    int free;                       // Denotes whether the KTP socket is free or not
    pid_t pid;                      // Process ID of creator of the socket
    int udp_sockid;                 // Corresponding internal UDP socket ID
    char ip_addr[INET_ADDRSTRLEN];  // IP Address of the destination
    uint16_t port;                  // Port number of the destination
    char send_buffer[10][512];      // Send buffer of the KTP socket
    int send_buffer_size;           // Current size of send buffer
    int sendbufferlen[10];          // Array to store length of each message in send buffer
    time_t last_send_time[256];     // Time at which message for ith seq number was last sent (Init with -1)
    char recv_buffer[10][512];      // Receive buffer of KTP socket
    int recv_buffer_live[10];       // 1 if ith message not read by application, 0 otherwise
    int rcvbufferlen[10];           // Array to store length of each message in receive buffer
    int recv_buffer_base;           // Pointer to location in receive buffer for application to read next
    window swnd;                    // Send window for KTP socket
    window rwnd;                    // Receive window for KTP socket
    int full;                       // Indicates whether receive buffer is full or not
} SHARED_MEM;



/* External variables */
extern SHARED_MEM *SM;                  // Pointer to shared memory
extern NET_SOCKET* net_socket;          // Pointer to network socket
extern struct sembuf pop, vop;          // Semaphore operations
extern int semid_SM, semid_net_socket;  // Semaphore IDs
extern int shmid_SM, shmid_net_socket;  // Shared memory IDs
extern int semid_init, semid_ktp;       // Additional semaphore IDs

// Function Prototypes
int k_socket(int domain,int type,int protocol);
int k_bind(char src_ip[],uint16_t src_port,char dest_ip[],uint16_t dest_port);
ssize_t k_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t k_recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr * src_addr, socklen_t *addrlen);
int k_close(int sockfd);
int drop_Message();

#endif
