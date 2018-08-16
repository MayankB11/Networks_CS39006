/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <iostream>
#include <string>
#include <cmath>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
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

/*
        Method to compute the size of file  
*/
long int get_file_size(char* filename) // path to file
{
    FILE *f = NULL;
    f = fopen(filename,"r");
    if(f==NULL){
        cout<<"File doesn't exist"<<endl;
        exit(1);
    }
    fseek(f,0,SEEK_END);
    long int size = ftell(f);
    fclose(f);
    return size;                // returns size of file
}

void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char **argv) {
    int sockfd, portno, n, retransmitted=0;
    socklen_t serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];

    /* check command line arguments */
    if (argc != 4) {
       fprintf(stderr,"usage: %s <hostname> <port> <filename>\n", argv[0]);
       exit(0);
    }
    cout<<"--------- Client Running ------------"<<endl;
    hostname = argv[1];
    portno = atoi(argv[2]);
    char* filename = new char;
    cout<<"File Name : "<<argv[3]<<endl;                
    strcpy(filename,argv[3]);                           // copying fileName
    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) 
        error("ERROR opening socket");
    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    struct timeval timeout;      
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0)
        error("setsockopt failed\n");


    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
      (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* get a message from the user */
    bzero(buf, BUFSIZE);
    //printf("Please enter file name: ");
    //fgets(buf, BUFSIZE, stdin);
    int total_chunks;
    long int file_size = get_file_size(filename);                                 // Getting fileSize
    total_chunks = ceil(file_size/1024.0);
    cout<<"Total Chunks : "<<total_chunks<<"\nFile Size : "<<file_size<<endl;
    string str = string(filename)+" "+to_string(file_size)+" "+to_string(total_chunks);         // Creating Hello message to be transfered
    cout<< " Msg Sent : "<<str<<endl;
    serverlen = sizeof(serveraddr);
    
    /* send the message to the server */
    bzero(buf,BUFSIZE);
    strcpy(buf,str.c_str());
    n = sendto(sockfd, buf, BUFSIZE, 0,(sockaddr*) &serveraddr, serverlen);
    if (n < 0) 
      error("ERROR in sendto");
    bzero(buf,BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0,(sockaddr*)  &serveraddr, &serverlen);
    if(strcmp(buf,"ACK")!=0){
        cout<<"ACK NOT RECEIVED"<<endl;
    }
    FILE* f = fopen(filename,"rb");
    if(f == NULL){
        error("File not found");
    }
    long int bytes_sent  = 0;
    int sequence_no = 0;
    msg m;

    int nr,ns,size_read;
    while(bytes_sent < file_size){
        bzero(m.chunk,1024);
        size_read = fread(m.chunk,sizeof(char), sizeof(m.chunk), f);
        m.sequence_no = sequence_no;
        m.length_of_chunk = size_read;
        while(1){
            ns = sendto(sockfd,(struct msg*) &m , sizeof(m), 0,(sockaddr*) &serveraddr, serverlen);

            if (ns < 0){
                cout<<"Error in Sending"<<endl;
                continue;
            }

            nr = recvfrom(sockfd, (struct msg*)&m, sizeof(m), 0,(sockaddr*)  &serveraddr, &serverlen);
            if(nr < 0){
            	retransmitted++;
                cout<<"Error in Receving"<<endl;
                continue;
            }
            else if(sequence_no == m.sequence_no){
                cout<<" ACK "<<sequence_no<<" received"<<endl;
                break;
            }
            else{
                retransmitted++;
                cout<<" Wrong ACK received"<<endl;
            }
        }
        bytes_sent += size_read;
        sequence_no++;
    }
    cout<<"Retransmitted Packets "<<retransmitted<<endl;
    char** temp=new char*[3];                                               // Stores command to execute to compute md5sum of file
    temp[0] = new char;
    temp[1] = new char;
    temp[2] = new char;
    temp[0]="md5sum";
    strcpy(temp[1],filename);
    temp[2] = NULL;
   // cout<<"Here"<<endl;
    int file_desc = open("md5sum.txt",O_CREAT|O_WRONLY,0666);              // file_desc for writing the md5sum output
    pid_t pid = fork();
    if(pid==0){                                                         // creating child process for computing md5sum
        dup2(file_desc,1);                                                 // redirecting stdout to md5sum.txt
        execvp(temp[0],temp);                                              // computing md5sum
    }
   // cout<<"Here"<<endl;
    int status;

    waitpid(pid,&status,0);                                                          // wait for the child process to compute md5sum
    sleep(2);
    close(file_desc);
    fclose(f);
    ifstream f1;
    bzero(buf,BUFSIZE);
    f1.open("md5sum.txt");
    char buf1[BUFSIZE];
    bzero(buf1,BUFSIZE);
    f1>>buf1;                                                              // input the md5sum computed for client file
    n =recvfrom(sockfd,buf,33,0,(sockaddr*)  &serveraddr, &serverlen);                                              // receive the md5 sum computed by server
    cout<<"MD5 Received"<<endl;
    f1.close();
    /* Print Matched or not Matched
    according to received md5sum
    */
    if(strcmp(buf,buf1)==0){
      cout<<"MD5 Matched : "<<buf<<endl;
    }
    else
      cout<<"MD5 Not Matched : \n"<<"Server : "<<buf<<"\nClient : "<<buf1<<endl;
    close(sockfd);
    return 0;
}
