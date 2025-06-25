/*===========================================*/
//Name-Diganta Mandal
//Roll-22CS30062
/*============================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ksocket.h"
#define BUFSIZE 512

int main(int argc, char *argv[]) {
    if(argc!=5){
      printf("[-] Usage: %s <src_ip> <src_port> <dest_ip> <dest_port>\n", argv[0]);
      exit(1);
    }

    int sockfd=k_socket(AF_INET,SOCK_KTP,0);
    if(sockfd<0){
      perror("[-] socket creation failed");
      exit(1);
    }
    printf("[+] Socket created\n");

    char src_ip[INET_ADDRSTRLEN], dest_ip[INET_ADDRSTRLEN];
    strcpy(src_ip, argv[1]);
    strcpy(dest_ip, argv[3]);
    int src_port=atoi(argv[2]);
    int dest_port=atoi(argv[4]);

    if(k_bind(src_ip,src_port, dest_ip, dest_port)<0){
      perror("[-] bind failed");
      exit(1);
    }
    printf("[+] Bind successful\n");
    char filename[100];
    strcpy(filename,"recv_");
    sprintf(filename+5,"%d.txt",dest_port);
    int fd=open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if(fd<0){
      perror("[-] open failed");
      exit(1);
    }

    struct sockaddr_in serveraddr;
    socklen_t addrlen=sizeof(serveraddr);
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port=htons(dest_port);
    serveraddr.sin_addr.s_addr=inet_addr(dest_ip);
    printf("[+] Receiving file...\n");
    char buffer[BUFSIZE];
    int recv_bytes;
    int cnt=0;
    while(1){
      while((recv_bytes=k_recvfrom(sockfd,buffer,BUFSIZE,0,(struct sockaddr*)&serveraddr,&addrlen))<=0){
        sleep(1);
      }
      printf("[+] Received %d bytes, packets received=%d!!!\n",recv_bytes,++cnt);
      if(buffer[0]=='#'){
        printf("[+] EOF received\n");
        break;
      }
      printf("[+] Writing %d bytes to file.\n", recv_bytes);

      if(write(fd,buffer,recv_bytes)<0){
        perror("[-] write failed");
        exit(1);
      }
    }

    close(fd);
    printf("[+] Task Completed.Shutting down...\n");
    if(k_close(sockfd)<0){
      perror("[-] close failed");
      exit(1);
    }
    return 0;
}


