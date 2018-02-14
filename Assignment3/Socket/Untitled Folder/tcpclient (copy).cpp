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
    return size;
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];

    /* check command line arguments */
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

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

    /* get message line from the user */
    printf("Please enter filename: ");
    bzero(buf, BUFSIZE);
    cin>>buf;
    char* filename = new char;
    strcpy(filename,buf);
    /* send the message line to the server */
    n = write(sockfd, buf, strlen(buf));

    if (n < 0) 
      error("ERROR writing to socket");

    /* print the server's reply */
    bzero(buf, BUFSIZE);
    n = read(sockfd, buf, BUFSIZE);
    if (n < 0) 
      error("ERROR reading from socket");

    printf("Echo from server: %s", buf);
    int file_size = get_file_size(buf);
    n = write(sockfd,(to_string(file_size)).c_str(),BUFSIZE);
    if(n< 0)
        error("Error writing to socket");

    int count = 0;
    FILE* f=fopen(buf,"r");
    char* a;
    while(1){
        count = fread(buf,sizeof(char),BUFSIZE,f);
        if(count == 0){
            break;
        }
        else if(count > 0){
            send(sockfd, buf, count, 0);
           // cout<<count<<" bytes sent"<<endl;
        }
    }
    char** temp=new char*[3];
    temp[0]="md5sum";
    strcpy(temp[1],filename);
    temp[2] = NULL;
    int file_desc = open("md5sum.txt",O_CREAT|O_WRONLY,0666);
    if(fork()==0){
        dup2(file_desc,1);
        execvp(temp[0],temp);
    }
    int status;
    wait(&status);
    sleep(2);
    close(file_desc);
    fclose(f);
    ifstream f1;
    bzero(buf,BUFSIZE);
    f1.open("md5sum.txt");
    char buf1[BUFSIZE];
    bzero(buf1,BUFSIZE);
    f1>>buf1;
    n =recv(sockfd,buf,33,0);
    f1.close();
    if(strcmp(buf,buf1)==0){
      cout<<"MD5 Matched"<<endl;
    }
    else
      cout<<"MD5 Not Matched"<<endl;
    cout<<n<<" : "<<buf<<endl;
    close(sockfd);
    return 0;
}
