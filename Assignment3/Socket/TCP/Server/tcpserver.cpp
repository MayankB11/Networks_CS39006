/* 
 * tcpserver.c - A simple TCP echo server 
 * usage: tcpserver <port>
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
#include <sys/stat.h>
#include <sys/wait.h>
#include <iostream>
#include <fcntl.h>
#include <fstream>
#define BUFSIZE 1024
using namespace std;
#if 0
/* 
 * Structs exported from in.h
 */

/* Internet address */
struct in_addr {
  unsigned int s_addr; 
};

/* Internet style socket address */
struct sockaddr_in  {
  unsigned short int sin_family; /* Address family */
  unsigned short int sin_port;   /* Port number */
  struct in_addr sin_addr;	 /* IP address */
  unsigned char sin_zero[...];   /* Pad to size of 'struct sockaddr' */
};

/*
 * Struct exported from netdb.h
 */

/* Domain name service (DNS) host entry */
struct hostent {
  char    *h_name;        /* official name of host */
  char    **h_aliases;    /* alias list */
  int     h_addrtype;     /* host address type */
  int     h_length;       /* length of address */
  char    **h_addr_list;  /* list of addresses */
}
#endif

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}



int main(int argc, char **argv) {
  int parentfd; /* parent socket */
  int childfd; /* child socket */
  int portno; /* port to listen on */
  socklen_t clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char fileName[BUFSIZE]; /* message buffer */
  char buf[BUFSIZE]; /* message buffer */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */

  /* 
   * check command line arguments 
   */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /* 
   * socket: create the parent socket 
   */
  parentfd = socket(AF_INET, SOCK_STREAM, 0);
  if (parentfd < 0) 
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));

  /* this is an Internet address */
  serveraddr.sin_family = AF_INET;

  /* let the system figure out our IP address */
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

  /* this is the port we will listen on */
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(parentfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  /* 
   * listen: make this socket ready to accept connection requests 
   */
  if (listen(parentfd, 5) < 0) /* allow 5 requests to queue up */ 
    error("ERROR on listen");
  printf("Server Running ....\n");
  /* 
   * main loop: wait for a connection request, echo input line, 
   * then close connection.
   */
  clientlen = sizeof(clientaddr);
  while (1) {

    /* 
     * accept: wait for a connection request 
     */
    sleep(60);
    childfd = accept(parentfd, (struct sockaddr *) &clientaddr, &clientlen);
    if (childfd < 0) 
      error("ERROR on accept");
    
    /* 
     * gethostbyaddr: determine who sent the message 
     */
/*    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
			  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("server established connection with %s (%s)\n", 
	   hostp->h_name, hostaddrp);*/
    
    /* 
     * read: read fileName from the client
     */
    bzero(buf, BUFSIZE);
    n = read(childfd, buf, BUFSIZE);
    if (n < 0) 
      error("ERROR reading from socket");
    printf("Server received %d bytes: %s\n", n, buf);   // Printing number of bytes received by server
    strcpy(fileName,buf);   

    string str(buf);
    int i,c=1;
    for(i=0;str[i]!=' ';i++){   // Finding the start fileName
      c++;
    }
    for(i=c;str[i]!=' ';i++){  // Finding the end of fileName
    }
    char* fileName=new char;
    strcpy(fileName,str.substr(c,i-c).c_str());  // Getting fileName from message

    strcpy(buf,"Hello Acknowledged");

    n = write(childfd, buf, strlen(buf));  // Sending Acknowledgement Message
    strcpy(buf,(str.substr(i+1,strlen(str.c_str())-i)).c_str());
    int fileSize = atoi(buf);             // Getting fileSize in int
    cout<<"FileSize : "<<fileSize<<endl;
    if (n < 0) 
      error("ERROR writing to socket");

    FILE* f = fopen(fileName,"w");       // Creating the file for writing
    int count = 0;
    char* a;
    while(1){      
      bzero(buf, BUFSIZE);                // Clear the buffer for reading new data
      n = recv(childfd, buf, BUFSIZE,0);  // Receiving data in the buffer  
      if(n < 0)
        error("ERROR reading from socket");
      count+=n;                          // Maintaining count of total data received in the buffer
     // cout<<"Count : "<<count<<endl;
      n = fwrite(buf, sizeof(char), n, f); // Wrting data to the file
     // cout<<n<<" bytes received, Total bytes received by now : "<<count<<endl;
      if(n == 0 || count == fileSize){
       //cout<<"n"<<n<<" count "<<count<<" fileSize "<<fileSize<<endl;
       fclose(f);
        
        break;                          // Exiting while loop on getting the complete file
      }
      if(n<0)
        error("ERROR writing to the file");    
    }
 
    char** temp=new char*[3];                                     // To store the command to compute md5sum of file
    temp[0]=new char;
    temp[1]=new char;
    temp[2]=new char;
    temp[0]="md5sum";
    strcpy(temp[1],fileName);
    temp[2] = NULL;
    int file_desc = open("md5sum.txt",O_CREAT|O_WRONLY,0666);   // file_desc for writing the md5sum output
    if(fork()==0){                                              // creating child process for computing md5sum
        dup2(file_desc,1);                                      // redirecting stdout to md5sum.txt
        execvp(temp[0],temp);                                   // computing md5sum
    }
    int stat;
    wait(&stat);                                              // wait for the child process to compute md5sum
   close(file_desc);
    ifstream f1;
    f1.open("md5sum.txt");
    char buf1[BUFSIZE];
    bzero(buf1,BUFSIZE);
    f1>>buf1;                                                // input the md5sum computed
    f1.close();
    n = send(childfd, buf1, BUFSIZE,0);                       // send the md5 sum to client
    cout<<"MD5 Sent : "<<buf1<<endl;
    close(childfd);                                           // close the child
  }
}
