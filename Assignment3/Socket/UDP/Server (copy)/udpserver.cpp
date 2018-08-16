/* 
 * udpserver.c - A UDP echo server 
 * usage: udpserver <port_for_server>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <fcntl.h>
#include <fstream>
#include <sys/wait.h>
using namespace std;
#define BUFSIZE 1024

struct msg{
    int sequence_no;
    int length_of_chunk;
    char chunk[1024];
};

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char **argv) {
  int sockfd; /* socket file descriptor - an ID to uniquely identify a socket by the application program */
  int portno; /* port to listen on */
  socklen_t clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */

  /* 
   * check command line arguments 
   */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port_for_server>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /* 
   * socket: create the socket 
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  /* 
   * main loop: wait for a datagram, then echo it
   */
  cout<<"------------ Server Running ------------"<<endl;
  clientlen = sizeof(clientaddr);
  msg m;
  while (1) {

    /*
     * recvfrom: receive a UDP datagram from a client
     */
    bzero(buf,BUFSIZE);
    n = recvfrom(sockfd,buf, sizeof(buf), 0, (struct sockaddr *) &clientaddr, &clientlen);


    if (n < 0)
      error("ERROR in recvfrom");
    string str(buf);
    int i,c=1;
    for(i=0;str[i]!=' ';i++){   // Finding the start fileName
      c++;
    }
    for(i=c;str[i]!=' ';i++){  // Finding the end of fileName
    }
    char* fileName=new char;
    
    strcpy(fileName,str.substr(0,c-1).c_str());  // Getting fileName from message
    bzero(buf,BUFSIZE);
    
    strcpy(buf,(str.substr(c,i-c+1).c_str()));
    long int fileSize = atoi(buf);             // Getting fileSize in int
    cout<<"FileSize : "<<fileSize<<endl;
    
    bzero(buf,BUFSIZE);
    strcpy(buf,"ACK");
    n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *) &clientaddr, clientlen);
    if (n < 0) 
      error("ERROR in sendto");
    
    long int bytes_received = 0;
    int sequence_no = 0;
    int ns,nr;

    FILE *f = fopen(fileName,"w");
    cout<<"File Receiving "<<endl;
    while(bytes_received < fileSize){
      while(1){
      nr = recvfrom(sockfd, (msg*)&m, sizeof(m), 0, (struct sockaddr *) &clientaddr, (socklen_t *) &clientlen);
      if ( nr < 0){
        continue;        
      }
      msg m1;
        if(m.sequence_no == sequence_no){
          cout<<"Received Packet "<<sequence_no<<endl;
          m1.sequence_no = sequence_no;
          ns = sendto(sockfd, (msg*)&m1, sizeof(m1), 0, (struct sockaddr *) &clientaddr, clientlen);
          if(ns < 0){
            cout<<"Error in sending acknowledgement "<<sequence_no<<endl;
          }
          else{
            cout<<"ACK "<<sequence_no<<" sent"<<endl;
            break;
          }
        }
        else{
          m1.sequence_no = m.sequence_no;
          ns = sendto(sockfd, (msg*)&m1, sizeof(m1), 0, (struct sockaddr *) &clientaddr, clientlen);
          if(ns < 0){
            cout<<"Error sending ACK "<<sequence_no<<endl;
          }
        }
      }
      fwrite(m.chunk,sizeof(char),m.length_of_chunk,f);
      bytes_received+=m.length_of_chunk;
      sequence_no++;
    }
    cout<<"File bytes transfered : "<<bytes_received<<endl;
    cout<<"FILE TRANSFER COMPLETE :) "<<endl;
    fclose(f);
    
    char** temp=new char*[3];                                     // To store the command to compute md5sum of file
    temp[0]=new char;
    temp[1]=new char;
    temp[2]=new char;
    temp[0]="md5sum";
    strcpy(temp[1],fileName);
    temp[2] = NULL;
    int file_desc = open("md5sum.txt",O_CREAT|O_WRONLY,0666);   // file_desc for writing the md5sum output
    pid_t pid = fork();
    if(pid==0){                                              // creating child process for computing md5sum
        dup2(file_desc,1);                                      // redirecting stdout to md5sum.txt
        execvp(temp[0],temp);                                   // computing md5sum
    }
   int stat;
   waitpid(pid,&stat,0);                                             // wait for the child process to compute md5sum
    close(file_desc);
    ifstream f1;
    f1.open("md5sum.txt");
    char buf1[BUFSIZE];
    bzero(buf1,BUFSIZE);
    f1>>buf1;                                                // input the md5sum computed
    f1.close();
    n = sendto(sockfd, buf1, BUFSIZE,0,(struct sockaddr *) &clientaddr, clientlen);                       // send the md5 sum to client
    cout<<"MD5 Sent : "<<buf1<<endl;
    //close(childfd);                                           // close the child
    /* 
     * gethostbyaddr: determine who sent the datagram
    
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
        sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("server received datagram from %s (%s)\n", 
     hostp->h_name, hostaddrp);
    printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);
    */
    
    /* 
     * sendto: echo the input back to the client 
     */
  }
}
