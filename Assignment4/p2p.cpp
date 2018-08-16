#include <bits/stdc++.h>
#include <sys/time.h>

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

using namespace std;

#define STDIN 0
/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}


struct ip_port{			// Structure to Store IP Address and PORT
	string ip;			// For IP
	int port;			// For Port
};

struct conn{			// Structure to store name, ip details and time last interacted
	string name;		// For Username
	ip_port det;		// For IP details
	time_t last;		// For time last interacted
};


int main(int argc, char **argv)
{
	map <string, ip_port> user_info;		// Map to store details of username
	ifstream f("user_info.txt");			// Reading the file containg user ip details
	
	while(!f.eof())
	{
		string s1,s2,s3;
		f>>s1>>s2>>s3;							// Reading from file
		user_info[s1].ip=s2;					// Storing IP
		user_info[s1].port=atoi(s3.c_str());	// Storing Port
	} 
	map <string,ip_port> :: iterator it;
	for(it=user_info.begin();it!=user_info.end();it++)
	{
		cout<<it->first<<" "<<it->second.ip<<" "<<it->second.port<<endl;
	}


	int parentfd;							// Stores file descriptor of the server of the current instance of executable running
	int portno;								// Stores port no of the server of the current instance of executable running
	int optval;
  	int newfd;									
  	int sockfd;
	struct sockaddr_in serveraddr;
	socklen_t addrlen; /* byte size of client's address */
	struct sockaddr_in clientaddr; /* client addr */
	addrlen = sizeof(clientaddr);
	struct sockaddr_in remoteaddr;
	struct hostent *server;

  /* 
   * check command line arguments 
   */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);					// Stores port number from input

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

  	fd_set master;					// Stores set of all file descriptors
  	fd_set read_fds;				// Stores set of all file descriptors waiting to be read
  	int fdmax;						// Value of maximum file descriptor
  	FD_ZERO(&master);				// Initialize master 
  	FD_ZERO(&read_fds);				// Initialize read_fds

  	FD_SET(parentfd,&master);	    // Add file descriptor of the current running server to master
  	FD_SET(STDIN,&master);			// Add stdin to master

  	fdmax = parentfd;				// Initialize fdmax

  	struct timeval tv;				// To store timeout value
  	tv.tv_sec = 1;
  	tv.tv_usec = 0;

  	map < int, conn > fd_name;		// Stores map of file descriptor to connection details
  	string combined;
  	string name;
  	string message;

  	while(1)
  	{
//  		cout<<"WHILE"<<endl;
  		read_fds = master ;
  		tv.tv_sec = 1;
  		tv.tv_usec = 0;
  		int k =select(fdmax+1,&read_fds,NULL,NULL,&tv); // Select to check the file descriptors waiting to be read
  		if(k <= 0)
  		{

  		}
  		else
  		{
//  			cout<<"ELSE"<<endl;
  			for ( int i = 0; i <= fdmax; i++)			// Iterating to fdmax 
  			{
  				if(FD_ISSET(i,&read_fds)) 				// Checking if the file descriptor is ready to be read
  				{
//  					cout<<i<<"<--"<<endl;
  					if(i == parentfd)					// Checks if the read ready is the current connection
  					{
  						addrlen = sizeof(clientaddr);
  						newfd = accept(parentfd,(struct sockaddr*)&clientaddr,&addrlen);		// Accept the new incoming connection
  						if(newfd < 0)
  						{
  							perror("Accept");
  						}
//  						cout<<newfd<<endl;
  //						cout<<inet_ntoa(clientaddr.sin_addr)<<endl;
  						map <string,ip_port> :: iterator it;
  						for( it = user_info.begin();it!=user_info.end();it++)			// loop to find the user details of the incoming connection
  						{
  							if(it->second.ip == inet_ntoa(clientaddr.sin_addr))
  								break;
  						;	
  						}
  						if(it==user_info.end()){		// If user is not present
  							cout<<"Invalid Username"<<endl;
  							continue;
  						}
  						cout<<it->first<<" connected"<<endl;
  						FD_SET(newfd,&master);			// Add new connection to file descriptors selections
  						fdmax = max(fdmax,newfd);		// Recompute the fdmax
  						fd_name[newfd].name=it->first;
  						fd_name[newfd].det = it->second;  						
  						time(&fd_name[newfd].last); 
/*
  						char buff[256];
  						bzero(buff,256);
  						int n = read(newfd,buff,255);
  						cout<<buff<<endl;
  						cout<<"!"<<endl;	*/
  					}
  					else
  					{
  						if(i == STDIN)	// Check if input is being read
  						{

  							getline(cin,combined);	// Get the input from command line
  							int i;
 // 							cout<<combined<<endl;
  							for(i = 0; i < combined.size();i++)	// Extracting the message
  							{
  								if(combined[i]=='\\')
  									break;
  							}

  							if(i==combined.size()){  // Checing if the message was correct type
  								cout<<"Invalid Format"<<endl<<"Correct format: username\\msg"<<endl;
  								continue;
  							}
  							name = combined.substr(0,i);
  							message = combined.substr(i+1,combined.length()-i-1);
  							map < int, conn > :: iterator it;
  //							cout<<name<<"--"<<message<<endl;
  							for(it = fd_name.begin();it != fd_name.end(); it++)		// Finding the ip details using name
  							{
  								if(it->second.name == name)
  									break;
  							}
  							int new_port;
						   	char buf[256];
						   	bzero(buf,256); 
						   	strcpy(buf,message.c_str());

  							if(it == fd_name.end())				// Checking if the connection already exists
  							{
  								// Creating a new connection
	  							if(user_info.find(name)!=user_info.end()){
	  								sockfd = socket(AF_INET, SOCK_STREAM, 0);	// Opening the socket
	  								new_port = user_info[name].port;
	  								if (sockfd < 0) 							// Checking if the socket is open
	        							error("ERROR opening socket");			
	        						server = gethostbyname(user_info[name].ip.c_str());
								    if (server == NULL) {
								        fprintf(stderr,"ERROR, no such host\n");
								        exit(0);
								    }
								    struct sockaddr_in serv_addr;
								    bzero((char *) &serv_addr, sizeof(serv_addr));
								    serv_addr.sin_family = AF_INET;
								    bcopy((char *)server->h_addr, 
								         (char *)&serv_addr.sin_addr.s_addr,
								         server->h_length);
								    serv_addr.sin_port = htons(portno);
								    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) // Creating the connection
								        error("ERROR connecting"); 
								    
								    FD_SET(sockfd,&master);					// Add the file descriptor of the created connection to the master
								    fdmax = max(sockfd,fdmax);
								    fd_name[sockfd].name = name;
								    fd_name[sockfd].det = user_info[name];
								    time(&fd_name[sockfd].last);

								   	
								   	int n = send(sockfd,buf,message.size(),0);
								    if(n<0)
								    {
								    	error("send");
								    }  
								}
								else{
									// In case username doesn't exist
									cout<<"Invalid Username"<<endl;
								} 		
  							}
  							else
  							{
  								// Connection is already there, send the message
  								sockfd = it->first;
							   	int n = send(sockfd,buf,message.size(),0);
							    if(n<0)
							    {
							    	error("send");
							    }     
							    time(&fd_name[sockfd].last);

  							}

  						}
  						else
  						{
  							// Receive the message from other connection 
  							char buf[256];
  							bzero(buf,256);
  							int n = recv(i,buf,256,0);
  							if(n==0)
  							{
  								// Connection is closed
  								cout<<fd_name[i].name<<" closed"<<endl;
  								close(i);
  								FD_CLR(i,&master);
  								fd_name.erase(i);
  								// Removed the instances of the connection
  							}

  							else if(n<0)
  							{	// Error in receiving the message
  								error("Receive");

  							}
  							else
  								cout<<fd_name[i].name<<" : "<<buf<<endl; // Display the message received
;

  						}
  					}
  				}
  			}
  		}

  		time_t cur;
  		map < int, conn > :: iterator it;
  		for(it = fd_name.begin();it != fd_name.end(); it++) // Loop to erase connections that have been idle for more than 10 sec
  		{
  			time(&cur);						// Stores the current time in cur
  			if(cur - it->second.last >= 10)
  			{
  				close(it->first);			// closing the connection
  				FD_CLR(it->first,&master);	// remove the file descriptor from master
  				fd_name.erase(it->first);	
  			}
  		}
  	}
	return 0;
}