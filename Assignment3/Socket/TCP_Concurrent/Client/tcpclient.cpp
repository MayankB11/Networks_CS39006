/* 
 * tcpclient.c - A simple TCP client
 * usage: tcpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <iostream>
#include <string>
#include <fstream>

#define BUFSIZE 1024

using namespace std;
/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

/*
        Method to compute the size of file  
*/
int get_file_size(char* filename) // path to file
{
    FILE *f = NULL;
    f = fopen(filename,"r");
    if(f==NULL){
        cout<<"File doesn't exist"<<endl;
        exit(1);
    }
    fseek(f,0,SEEK_END);
    int size = ftell(f);
    fclose(f);
    return size;                // returns size of file
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];

    /* check command line arguments */
    if (argc != 4) {
       fprintf(stderr,"usage: %s <hostname> <port> <filename>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);
    char* filename = new char;
    cout<<"File Name : "<<argv[3]<<endl;                
    strcpy(filename,argv[3]);                           // copying fileName
    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* connect: create a connection with the server */
    if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) 
      error("ERROR connecting");

    cout<<"----------Client Running----------\n";
    /* send the message line to the server */
	cout<<n<<endl;
    int file_size = get_file_size(filename);                                 // Getting fileSize
    string str = "Hello "+string(filename)+" "+to_string(file_size);         // Creating Hello message to be transfered
    n = write(sockfd, str.c_str(), strlen(str.c_str()));                     // sending hello message
    if(n< 0)
        error("Error writing to socket");
    else{
        cout<<"Client Hello message sent.\nMessage contents : "<<str<<endl;
    }

    /* print the server's reply */
    bzero(buf, BUFSIZE);
    n = read(sockfd, buf, BUFSIZE);                                          // Reading the acknowledgement sent from server
    if (n < 0) 
      error("ERROR reading from socket");

    printf("Echo from server: %s\n", buf);

    int count = 0;
    int total_count = 0 ;
    FILE* f=fopen(filename,"r");                                            // Opening the file to be sent
    if(f == NULL){
        cout<<"FILE NOT FOUND"<<endl;
        exit(1);
    }
    else{
        cout<<"Transferring FILE : "<<filename<<endl;
    }
    char* a;
    while(1){
        count = fread(buf,sizeof(char),BUFSIZE,f);                          // Getting data to send
        if(count == 0 || total_count == file_size){
            break;
        }
        else if(count > 0){
            send(sockfd, buf, count, 0);                                   // Sending the data
            total_count+=count;
            cout<<count<<" bytes sent, Total bytes by now : "<<total_count<<endl;
            if(total_count == file_size){
                break;
            }
        }
    }
    char** temp=new char*[3];                                               // Stores command to execute to compute md5sum of file
    temp[0] = new char;
    temp[1] = new char;
    temp[2] = new char;
    temp[0]="md5sum";
    strcpy(temp[1],filename);
    temp[2] = NULL;
   // cout<<"Here"<<endl;
    int file_desc = open("md5sum.txt",O_CREAT|O_WRONLY,0666);              // file_desc for writing the md5sum output
    if(fork()==0){                                                         // creating child process for computing md5sum
        dup2(file_desc,1);                                                 // redirecting stdout to md5sum.txt
        execvp(temp[0],temp);                                              // computing md5sum
    }
   // cout<<"Here"<<endl;
    int status;
    wait(&status);                                                          // wait for the child process to compute md5sum
    sleep(2);
    close(file_desc);
    fclose(f);
    ifstream f1;
    bzero(buf,BUFSIZE);
    f1.open("md5sum.txt");
    char buf1[BUFSIZE];
    bzero(buf1,BUFSIZE);
    f1>>buf1;                                                              // input the md5sum computed for client file
    n =recv(sockfd,buf,33,0);                                              // receive the md5 sum computed by server
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
