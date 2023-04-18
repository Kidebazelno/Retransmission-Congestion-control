/* NOTE: The sample code only provides packet formats. No congestion control or go-back-N checking. */
/* NOTE: UDP socket is connectionless. Thus, the target IP and port should be specified whenever sending a packet. */
/* HINT:  We suggest using "bind," "sendto," and "recvfrom" in system/socket.h. */

/*
 * Connection Rules:
 * In this homework, each "agent," "sender," and "receiver" will bind a UDP socket to their ports, respectively, for sending and receiving packets.
 * When "agent" receives a packet, it will identify whether the packet sender is "sender" or "receiver" by the packet sender's IP and port.
 * When "sender" or "receiver" receives a packet, the packet sender is always "agent."
 * When "sender" or "receiver" wants to send a packet, they should know the IP and ports for "agent" first and send a packet by socket previously bound.
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include<errno.h>
#include "opencv2/opencv.hpp"
#include <zlib.h>
#include<sys/poll.h>

typedef struct {
	int length;
	int seqNumber;
	int ackNumber;
	int fin;
	int syn;
	int ack;
    unsigned long checksum;
} HEADER;

typedef struct{
	HEADER header;
	char data[1000];
} SEGMENT;
cv::Mat client_img;
int recv_bufsize = 256,order=1,num_elem=0;
int height=-1,width=-1;
int imgSize;
SEGMENT recv_buffer[256];
uchar *buffer;
uchar *buf_ptr;

void setIP(char *dst, const char *src){
    if(strcmp(src, "0.0.0.0") == 0 || strcmp(src, "local") == 0 || strcmp(src, "localhost") == 0){
        sscanf("127.0.0.1", "%s", dst);
    }
    else{
        sscanf(src, "%s", dst);
    }
    return;
}

uchar *iptr ;
void flush()
{
    
    for(int i=1;i<=num_elem;i++)
    {
        
        if(width==-1)
        {
            height=recv_buffer[1].header.length;
            width=recv_buffer[2].header.length;
            fprintf(stderr,"width:%d height:%d\n",width,height);
            client_img = cv::Mat::zeros(height, width, CV_8UC3);
            if(!client_img.isContinuous()){
                client_img = client_img.clone();
            }
            imgSize=client_img.total() * client_img.elemSize();
            fprintf(stderr,"imgSize:%d\n",imgSize);
            i=3;
            buffer = (uchar*)malloc(sizeof(uchar)*imgSize+1);
            buf_ptr = buffer;
            iptr=client_img.data;
            
        }
        memcpy(buf_ptr,recv_buffer[i%256].data,recv_buffer[i%256].header.length);
        
        
        
        if(recv_buffer[i%256].header.syn==1)
        {
            memcpy(client_img.data,buffer,imgSize);
            
            buf_ptr=buffer;
            imshow("Video", client_img);  
            char c = (char)cv:: waitKey(1000);
        }
        else buf_ptr+=recv_buffer[i%256].header.length;

    }
    
}

int main(int argc, char* argv[]){
    int recvsocket, portNum, nBytes;
    
    float error_rate;
    SEGMENT s_tmp,to_send;
    struct sockaddr_in sender, agent, receiver, tmp_addr;
    socklen_t agent_size, recv_size, tmp_size;
    char sendIP[50], agentIP[50], recvIP[50], tmpIP[50];
    int sendPort, agentPort, recvPort;
    
    if(argc != 3){
        fprintf(stderr,"Usage: %s <receiver port> <agent IP>:<agent port>\n", argv[0]);
        fprintf(stderr, "E.g., ./receiver <receiver port> <agent IP>:<agent port>\n");
        exit(1);
    }
    else{
        sscanf(argv[1], "%d", &recvPort);               // agent
        setIP(recvIP, "local");

        sscanf(argv[2], "%[^:]:%d", tmpIP, &agentPort);   // sender
        setIP(agentIP, tmpIP);
    }

    {/* Create UDP socket */
    recvsocket = socket(PF_INET, SOCK_DGRAM, 0);

    /* Configure settings in sender struct */
    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(recvPort);
    receiver.sin_addr.s_addr = inet_addr(recvIP);
    memset(receiver.sin_zero, '\0', sizeof(receiver.sin_zero));  

    /* Configure settings in agent struct */
    agent.sin_family = AF_INET;
    agent.sin_port = htons(agentPort);
    agent.sin_addr.s_addr = inet_addr(agentIP);
    memset(agent.sin_zero, '\0', sizeof(agent.sin_zero));    

    /* bind socket */
    bind(recvsocket,(struct sockaddr *)&receiver,sizeof(receiver));


    /* Initialize size variable to be used later on */
    agent_size = sizeof(agent);
    recv_size = sizeof(receiver);
    tmp_size = sizeof(tmp_addr);

    printf("Start!! ^Q^\n");
    printf("receiver info: ip = %s port = %d\n",
        recvIP, recvPort);
    printf("agent info: ip = %s port = %d\n", agentIP, agentPort);}

    int segment_size, index;
    char ipfrom[1000];
    char *ptr;
    int portfrom;

    // while(1){
    //     uchar *iptr = client_img.data;
    //     //memcpy(iptr, buffer, imgSize);
    //     recv(sockfd,buffer,imgSize,0);
    //     fprintf(stderr,"%.30s\n",buffer);
    //     memcpy(iptr, buffer, imgSize);
    //     //imshow("Video", client_img);  
    //     char c = (char)cv:: waitKey(30);
    //     if(c==27)
    //         break;
        
    // }
    while(1){
        /* Receive message from receiver and sender */
        memset(&s_tmp, 0, sizeof(s_tmp));
        memset(&to_send,0,sizeof(to_send));
        to_send.header.ack=1;
        to_send.header.fin=0;
        segment_size = recvfrom(recvsocket, &s_tmp, sizeof(s_tmp), 0, (struct sockaddr *)&tmp_addr, &tmp_size);
        if(segment_size > 0){
            inet_ntop(AF_INET, &tmp_addr.sin_addr.s_addr, ipfrom, sizeof(ipfrom));
            portfrom = ntohs(tmp_addr.sin_port);
            
            index = s_tmp.header.seqNumber;
            to_send.header.seqNumber = index;
            if(index!=order)
            {
                //out of order
                if(s_tmp.header.fin==1)
                {
                    printf("drop\tfin\t#%d\t\t(out of order)\n",index);
                }
                printf("drop\tdata\t#%d\t\t(out of order)\n",index);
                printf("send\tack\t#%d\n",order-1);
                to_send.header.ackNumber=order-1;
                
                sendto(recvsocket, &to_send, sizeof(to_send), 0, (struct sockaddr *)&agent, agent_size);
            }
            else
            {
                if(num_elem==256)
                {
                    //buf overflow
                    printf("drop\tdata\t#%d\t\t(buffer overflow)\n",index);
                    flush();
                    printf("send\tack\t#%d\n",order-1);
                    to_send.header.ackNumber=order-1;
                    
                    sendto(recvsocket, &to_send, sizeof(to_send), 0, (struct sockaddr *)&agent, agent_size);
                    printf("flush\n");
                    num_elem=0;
                    
                }
                else if(crc32(0L, (const Bytef *)s_tmp.data, 1000)!=s_tmp.header.checksum)
                {//corrupt
                    printf("drop\tdata\t#%d\t\t(corrupt)\n",index);
                    printf("send\tack\t#%d\n",order-1);
                    to_send.header.ackNumber=order-1;

                    sendto(recvsocket, &to_send, sizeof(to_send), 0, (struct sockaddr *)&agent, agent_size);
                }
                else if(s_tmp.header.fin!=1)
                {//ok
                    printf("recv\tdata\t#%d\n", order);
                    
                    to_send.header.ackNumber=order;
                    memcpy(&(recv_buffer[order%256]),&s_tmp,sizeof(s_tmp));
                    printf("send\tack\t#%d\n",order);
                    sendto(recvsocket, &to_send, sizeof(to_send), 0, (struct sockaddr *)&agent, agent_size);
                    order++;
                    num_elem++;
                }
                else
                {
                    printf("recv\tfin\n");
                    to_send.header.ackNumber=order;
                    to_send.header.fin=1;
                    printf("send\tfinack\n");
                    sendto(recvsocket, &to_send, sizeof(to_send), 0, (struct sockaddr *)&agent, agent_size);
                    
                    break;
                }
            }
        }
    }
    cv::destroyAllWindows();

    return 0;
}
