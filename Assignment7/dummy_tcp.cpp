#include "dummy_tcp.h"


dummy_tcp::dummy_tcp(int sockfd, struct sockaddr_in* serveraddr = NULL)
{
	if(serveraddr != NULL)
		{
			this->addr = *serveraddr;
			this->addrlen = sizeof(this->addr);
		}
	this->sockfd = sockfd;
	this->excepted_seq_no = 0;

	cwnd = 3*MSS;
	window = min(cwnd,recvw);

	//pthread_create(&sender,NULL);
	pthread_create(&reciever,NULL, &dummy_tcp::udp_recieve_helper, this);

}


void* dummy_tcp::parse_packets(packet_t *packet)
{
	if(packet->header.type == true)
	{
		//update_window(packet);
	}
	else
	{
		reciever_buffer_handler(packet);		
	}
}

void* dummy_tcp::udp_recieve(void* arg)
{
	char data[MSS];
	int n;
	packet_t* packet; 
	while(1){
		n = recvfrom(sockfd,data,MSS, 0, (struct sockaddr *) &addr, &addrlen);
		if(n < 0)
		{
			cout << "Error in recieve" << endl;
			exit(-1);
		}
		packet = (packet_t*)data;
		parse_packets(packet);
	}
}

void* dummy_tcp::reciever_buffer_handler(packet_t *packet)
{
	if(packet->header.seq == excepted_seq_no)
		{
			while(packet->header.length > (MAX_BUFFERSIZE-size));
			//acquire lock
			for(int i=0; i<packet->header.length; i++)
				reciever_data_buffer.push_back(packet->data[i]);
			excepted_seq_no+=packet->header.length;
			//release lock
			send_ack(excepted_seq_no-1);
			packet_t tmp;
			while(reciever_packet_queue.find(excepted_seq_no)!=reciever_packet_queue.end())
			{
				tmp = reciever_packet_queue[excepted_seq_no];
				while(tmp.header.length > (MAX_BUFFERSIZE-size));
				//acquire lock
				for(int i=0; i<tmp.header.length; i++)
					reciever_data_buffer.push_back(tmp.data[i]);
				reciever_packet_queue.erase(excepted_seq_no);
				excepted_seq_no+=tmp.header.length;
				//release lock
				send_ack(excepted_seq_no-1);
			}
		}
	else if(packet->header.seq < excepted_seq_no)
	{
		send_ack(excepted_seq_no-1);
	}
	else
	{
		reciever_packet_queue[packet->header.seq] = *packet;
		send_ack(excepted_seq_no-1);
	}
}

void* dummy_tcp::send_ack(int ackno)
{	
	bzero(data,MSS);
	packet_t* packet = (packet_t*)data;
	packet->header.seq = ackno;
	packet->header.type = true;
	packet->header.rw = MAX_BUFFERSIZE-size;
	int n = sendto(sockfd,data,MSS,0,(struct sockaddr *)&addr, addrlen);
	if(n<0)
	{
		cout << "Ack not sent" << endl;
	}
}	

void* dummy_tcp::create_packet(char* buf, int len, int seq)
{
	char* segment = new char[MSS];
	bzero(segment,MSS);
	header_t* header = (header_t*)segment;
	header->seq = seq;
	header->type = false;
	header->length = len;
	header->rw = this->rwnd;
	packet_t* packet = (packet_t*)segment;
	bcopy(packet->data,buf,len);	
}


int dummy_tcp::app_send(char* data,int len)
{
	while(!sender_buffer_handler(data,len));
}
int dummy_tcp::sender_buffer_handler(char* data, int len)
{
	if(this->sender_data_buffer.size()+len < MAX_BUFFERSIZE)
	{
		for(int i = 0 ; i < len ; i ++)
		{
			this->sender_data_buffer.push_back(data[i]);
		}
		return 1;
	}
	else 
		return 0;
}

int dummy_tcp::udp_send(char* packet,int len)
{
	int n;
	while(n<=0)
		n = sendto(this->sockfd,packet,len,0,(struct sockaddr*)&serveraddr,serverlen);
	return 1;
}

void dummy_tcp::update_window(packet_t* packet)
{
	this->rwnd = packet->header.rw;
	int tempack = packet->header.seq;
	if(tempack == this->cur_ptr){
		this->dupack++;
		if(this->dupack>=3){
			this->ssthresh = this->ssthresh/2;
			this->cwnd = this->ssthresh;
			this->dupack = 0;
		}
	}
	else if(tempack > cur_ptr){
		this->dupack = 0;
		this->cur_ptr = tempack;
		if(this->cwnd < this->ssthresh){
			this->cwnd += MSS;
		}
		else
			this->cwnd += MSS/this->cwnd;
	}
}

