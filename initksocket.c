/*===========================================*/
//Name-Diganta Mandal
//Roll-22CS30062
/*============================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <signal.h>
#include <pthread.h>
#include "ksocket.h"


int transmission;

void create_and_bind(){
  while(1){
    P(semid_init);
    P(semid_net_socket);
    //printf("Started...\n");
    if(net_socket->sock_id==0 && net_socket->port==0){
      int sockfd=socket(AF_INET,SOCK_DGRAM,0);
      if(sockfd<0){
        net_socket->sock_id=-1;
        net_socket->err_code=errno;
      }else{
        net_socket->sock_id=sockfd;
      }
    }
    else{
      struct sockaddr_in serveraddr;
      serveraddr.sin_family=AF_INET;
      serveraddr.sin_port=htons(net_socket->port);
      serveraddr.sin_addr.s_addr=inet_addr(net_socket->ip_addr);
      if(bind(net_socket->sock_id,(struct sockaddr*)&serveraddr,sizeof(serveraddr))<0){
        net_socket->sock_id=-1;
        net_socket->sock_id=errno;
      }
    }
    V(semid_net_socket);
    V(semid_ktp);
    //printf("Done\n");
  }
}


void *GC(){
  while(1){
    sleep(T);
    P(semid_SM);
    for(int i=0;i<N;i++){
      if(SM[i].free==0){
        if(kill(SM[i].pid,0)==-1){
          printf("[GC THREAD]:Garbage collection triggered\n");
          SM[i].free=1;
        }
      }
    }
    V(semid_SM);
  }
}

void *R(){
  fd_set readfds;
  FD_ZERO(&readfds);
  int mxfd=0;
  while(1){
    fd_set read2fds=readfds;

    struct timeval tv;
    tv.tv_sec=T;
    tv.tv_usec=0;
    int ret=select(mxfd+1,&read2fds,NULL,NULL,&tv);
    if(ret==-1){
      perror("select");
    }
    if(ret==0){
      FD_ZERO(&readfds);
      P(semid_SM);
      mxfd=0;
      for(int i=0;i<N;i++){
        if(!SM[i].free){
          FD_SET(SM[i].udp_sockid,&readfds);
          if(SM[i].udp_sockid>mxfd)mxfd=SM[i].udp_sockid;
          if(SM[i].full==1 && SM[i].rwnd.size>0){
            SM[i].full=0;
          }
          int last_ack_seq=(SM[i].rwnd.start-1+MAX_SEQ_NUM)%MAX_SEQ_NUM;
          struct sockaddr_in clientaddr;
          clientaddr.sin_family=AF_INET;
          clientaddr.sin_port=htons(SM[i].port);
          clientaddr.sin_addr.s_addr=inet_addr(SM[i].ip_addr);
          char ack[13];
          ack[0]='0';//acknowledgement
          for(int j = 0; j < 8; j++){
            ack[8 - j] = ((last_ack_seq >> j) & 1) + '0';
          }
          for (int j = 0; j < 4; j++){
            ack[12 - j] = ((SM[i].rwnd.size >> j) & 1) + '0';
          }
          sendto(SM[i].udp_sockid,ack,13,0,(struct sockaddr*)&clientaddr,sizeof(clientaddr));
        }
      }
      V(semid_SM);
    }
    if(ret>0){
      P(semid_SM);
      for(int i=0;i<N;i++){
        if(FD_ISSET(SM[i].udp_sockid,&read2fds)){
          char buffer[531];
          struct sockaddr_in clientaddr;
          socklen_t len=sizeof(clientaddr);
          int n=recvfrom(SM[i].udp_sockid,buffer,531,0,(struct sockaddr*)&clientaddr,&len);
          if(drop_Message()){
            continue;
          }
          if(n<0){
            perror("recvfrom");
          }else{
            if(buffer[0]=='1'){
              
              //Data
              int seq=0;
              int len=0;
              //extract seq from buffer
              for(int j=1;j<=8;j++){
                seq=(seq<<1)|(buffer[j]-'0');
              }
              for(int j=9;j<=18;j++){
                len=(len<<1)|(buffer[j]-'0');
              }
              printf("[R THREAD]: Data is Received for seq=%d.\n",seq);

              if(seq==SM[i].rwnd.start){
                //in-order message is received
                int buff_idx=SM[i].rwnd.wnd[seq];
                memcpy(SM[i].recv_buffer[buff_idx],buffer+19,len);
                SM[i].recv_buffer_live[buff_idx]=1;
                SM[i].rwnd.size--;
                SM[i].rcvbufferlen[buff_idx]=len;
                while(SM[i].rwnd.wnd[SM[i].rwnd.start]>=0 && SM[i].recv_buffer_live[SM[i].rwnd.wnd[SM[i].rwnd.start]]==1){
                  SM[i].rwnd.start=(SM[i].rwnd.start+1)%MAX_SEQ_NUM;
                }
              }else{
                //out of order message is received
                if(SM[i].rwnd.wnd[seq]>=0 && SM[i].recv_buffer_live[SM[i].rwnd.wnd[seq]]==0){
                  //new message is received
                  int buff_idx=SM[i].rwnd.wnd[seq];
                  memcpy(SM[i].recv_buffer[buff_idx],buffer+19,len);
                  SM[i].rwnd.size--;
                  SM[i].rcvbufferlen[buff_idx]=len;
                  SM[i].recv_buffer_live[buff_idx]=1;
                }
              }
              if(SM[i].rwnd.size==0){
                SM[i].full=1;
              }
              int last_ack_seq=(SM[i].rwnd.start-1+MAX_SEQ_NUM)%MAX_SEQ_NUM;
              char ack[13];
              ack[0]='0';//acknowledgement
              for(int j = 0; j < 8; j++){
                ack[8 - j] = ((last_ack_seq >> j) & 1) + '0';
              }
              for (int j = 0; j < 4; j++){
                ack[12 - j] = ((SM[i].rwnd.size >> j) & 1) + '0';
              }
              sendto(SM[i].udp_sockid,ack,13,0,(struct sockaddr*)&clientaddr,sizeof(clientaddr));
              printf("[R THREAD]: ACK is sent for seq=%d.\n",last_ack_seq);
            }
            else{
              //ACK
              
              int seq=0;
              int rwnd_size=0;
              
              for(int j=1;j<=8;j++){
                seq=(seq<<1)|(buffer[j]-'0');
              }
              for(int j=9;j<=12;j++){
                rwnd_size=(rwnd_size<<1)|(buffer[j]-'0');
              }
              printf("[R THREAD]: ACK is received for seq=%d.\n",seq);

              // if(SM[i].swnd.wnd[seq]>=0){
                int base=SM[i].swnd.start;
                while(base!=(seq+1)%MAX_SEQ_NUM){
                  SM[i].swnd.wnd[base]=-1;
                  SM[i].last_send_time[base]=-1;
                  SM[i].send_buffer_size++;
                  base=(base+1)%MAX_SEQ_NUM;
                }
                SM[i].swnd.start=(seq+1)%MAX_SEQ_NUM;
              //}
              SM[i].swnd.size=rwnd_size;
            }
          }
        }
      }
      V(semid_SM);
    }
  }
}

void *S(){
  while(1){
    sleep(T/2);
    P(semid_SM);
    for(int i=0;i<N;i++){
      if(!SM[i].free){
        struct sockaddr_in serveraddr;
        serveraddr.sin_family=AF_INET;
        serveraddr.sin_port=htons(SM[i].port);
        serveraddr.sin_addr.s_addr=inet_addr(SM[i].ip_addr);
        int timeout=0;
        int seq=SM[i].swnd.start;
        
        while(seq!=(SM[i].swnd.start+SM[i].swnd.size)%MAX_SEQ_NUM){
          if(SM[i].last_send_time[seq]!=-1 && time(NULL)-SM[i].last_send_time[seq]>T){
            timeout=1;
            break;
          }
          seq=(seq+1)%MAX_SEQ_NUM;
        }
        
        if(timeout){
          printf("[S THREAD]: Timeout Detected!!!\n");
          seq=SM[i].swnd.start;
          while(seq!=(SM[i].swnd.start+SM[i].swnd.size)%MAX_SEQ_NUM){
            if(SM[i].swnd.wnd[seq]!=-1){
              char buffer[531];
              buffer[0]='1';//Data
              for(int j=0;j<8;j++){
                buffer[8-j]=((seq>>j)&1)+'0';
              }
              int len=SM[i].sendbufferlen[SM[i].swnd.wnd[seq]];
              for(int j=0;j<10;j++){
                buffer[18-j]=((len>>j)&1)+'0';
              }
              memcpy(buffer+19,SM[i].send_buffer[SM[i].swnd.wnd[seq]],len);
              sendto(SM[i].udp_sockid,buffer,len+19,0,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
              printf("[S THREAD]: Sending packet again, seq=%d.\n",seq);
              //printf("seq=%d\n",seq);
              SM[i].last_send_time[seq]=time(NULL);
              //printf("Sent %d\n",sent_bytes);
              transmission++;
            }
            seq=(seq+1)%MAX_SEQ_NUM;
          }
        }else{
          seq=SM[i].swnd.start;
          while(seq!=(SM[i].swnd.start+SM[i].swnd.size)%MAX_SEQ_NUM){
            if(SM[i].swnd.wnd[seq]!=-1 && SM[i].last_send_time[seq]==-1){
              //printf("Sending...%d\n",i);
              char buffer[531];
              buffer[0]='1';//Data
              for(int j=0;j<8;j++){
                buffer[8-j]=((seq>>j)&1)+'0';
              }
              int len=SM[i].sendbufferlen[SM[i].swnd.wnd[seq]];
              for(int j=0;j<10;j++){
                buffer[18-j]=((len>>j)&1)+'0';
              }
              memcpy(buffer+19,SM[i].send_buffer[SM[i].swnd.wnd[seq]],len);
              sendto(SM[i].udp_sockid,buffer,len+19,0,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
              printf("[S THREAD]: Sending new packet, seq=%d.\n",seq);
              transmission++;
              //printf("seq=%d\n",seq);
              SM[i].last_send_time[seq]=time(NULL);
              //printf("Sent %d\n",sent_bytes);
            }
            seq=(seq+1)%MAX_SEQ_NUM;
          }

        }
      }
    }
    V(semid_SM);
  }
}

void handler(int sig){
  if(sig==SIGINT){
    printf("Number of transmissions=%d\n",transmission);
    printf("Cleanup started...\n");
    shmdt(net_socket);
    shmdt(SM);
    shmctl(shmid_net_socket,IPC_RMID,NULL);
    shmctl(shmid_SM,IPC_RMID,NULL);
    semctl(semid_net_socket,0,IPC_RMID);
    semctl(semid_SM,0,IPC_RMID);
    semctl(semid_init,0,IPC_RMID);
    semctl(semid_ktp,0,IPC_RMID);
    printf("Cleanup completed.Shutting down...\n");
    exit(0);
  }
}




int main(){
  signal(SIGINT, handler);
  srand(time(NULL));
  transmission=0;
  pop.sem_num = 0;
  pop.sem_op = -1;
  pop.sem_flg = 0;

  vop.sem_num = 0;
  vop.sem_op = 1;
  vop.sem_flg = 0;

  int key_shmid_net_socket,key_semid_net_socket,key_shmid_SM,key_semid_SM,key_semid_init,key_semid_ktp;
  key_shmid_net_socket=ftok("/",'A');
  key_semid_net_socket=ftok("/",'B');
  key_shmid_SM=ftok("/",'C');
  key_semid_SM=ftok("/",'D');
  key_semid_init=ftok("/",'E');
  key_semid_ktp=ftok("/",'F');

  shmid_net_socket=shmget(key_shmid_net_socket,sizeof(NET_SOCKET),0666|IPC_CREAT);
  semid_net_socket=semget(key_semid_net_socket,1,0666|IPC_CREAT);
  shmid_SM=shmget(key_shmid_SM,sizeof(SHARED_MEM)*N,0666|IPC_CREAT);
  semid_SM=semget(key_semid_SM,1,0666|IPC_CREAT);
  semid_init=semget(key_semid_init,1,0666|IPC_CREAT);
  semid_ktp=semget(key_semid_ktp,1,0666|IPC_CREAT);

  
  semctl(semid_net_socket,0,SETVAL,1);
  semctl(semid_SM,0,SETVAL,1);
  semctl(semid_init,0,SETVAL,0);
  semctl(semid_ktp,0,SETVAL,0);

  net_socket=(NET_SOCKET*)shmat(shmid_net_socket,NULL,0);
  SM=(SHARED_MEM*)shmat(shmid_SM,NULL,0);


  for(int i=0;i<N;i++)SM[i].free=1;

  memset(net_socket,0,sizeof(NET_SOCKET));

  pthread_t Rthread,Sthread,GCthread;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_create(&Rthread,&attr,R,NULL);
  pthread_create(&Sthread,&attr,S,NULL);
  pthread_create(&GCthread,&attr,GC,NULL);
  printf("All threads created...[Press Ctrl+C to exit]\n");

  create_and_bind();
  return 0;
}


