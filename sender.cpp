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
	int fin;//finish
	int syn;//sync
	int ack;
    unsigned long checksum;
} HEADER;

typedef struct{
	HEADER header;
	char data[1000];
} SEGMENT;

void setIP(char *dst, const char *src){
    if(strcmp(src, "0.0.0.0") == 0 || strcmp(src, "local") == 0 || strcmp(src, "localhost") == 0){
        sscanf("127.0.0.1", "%s", dst);
    }
    else{
        sscanf(src, "%s", dst);
    }
    return;
}

int main(int argc, char* argv[]){
    int sendersocket, portNum, nBytes;
    int threshold=16,window_size=1;
    float error_rate;
    SEGMENT s_tmp;
    SEGMENT window[300];
    int ack_recv[300];//states: wait,ok
    for(int i=0;i<300;i++)
    {
        ack_recv[i]=0;
    }
    int win_start=1;
    struct sockaddr_in sender, agent, tmp_addr;
    socklen_t agent_size, tmp_size;
    char sendIP[50], agentIP[50], tmpIP[50];
    int sendPort, agentPort;
    
    if(argc != 4){
        fprintf(stderr,"Usage: %s <agent port> <sender IP>:<sender port> <receiver IP>:<receiver port> <error_rate>\n", argv[0]);
        fprintf(stderr, "E.g., ./agent 8888 local:8887 local:8889 0.3\n");
        exit(1);
    }
    else{
        sscanf(argv[1], "%d", &sendPort);               // sender
        setIP(sendIP, "local");

        sscanf(argv[2], "%[^:]:%d", tmpIP, &agentPort);   // agent
        setIP(agentIP, tmpIP);

        // sscanf(argv[3], "%[^:]:%d", tmpIP, &recvPort);   // file
        // setIP(recvIP, tmpIP);
        // filename = argv[3]

    }

    {/* Create UDP socket */
    sendersocket = socket(PF_INET, SOCK_DGRAM, 0);

    /* Configure settings in sender struct */
    sender.sin_family = AF_INET;
    sender.sin_port = htons(sendPort);
    sender.sin_addr.s_addr = inet_addr(sendIP);
    memset(sender.sin_zero, '\0', sizeof(sender.sin_zero));  

    /* Configure settings in agent struct */
    agent.sin_family = AF_INET;
    agent.sin_port = htons(agentPort);
    agent.sin_addr.s_addr = inet_addr(agentIP);
    memset(agent.sin_zero, '\0', sizeof(agent.sin_zero));    

    /* bind socket */
    bind(sendersocket,(struct sockaddr *)&sender,sizeof(sender));

    

    /* Initialize size variable to be used later on */
    agent_size = sizeof(agent);
    tmp_size = sizeof(tmp_addr);

    printf("Start!! ^Q^\n");
    printf("sender info: ip = %s port = %d\n",
        sendIP, sendPort);
    printf("agent info: ip = %s port = %d\n", agentIP, agentPort);}

    int segment_size, index;
    char ipfrom[1000];
    char *ptr;
    int portfrom;

    cv::Mat server_img;
    cv::VideoCapture cap(argv[3]);
    int width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    // send(sendersocket,&width,sizeof(int),0);
    // send(sendersocket,&height,sizeof(int),0);
    
    fprintf(stderr,"%d %d\n",height,width);
    server_img = cv::Mat::zeros(height, width, CV_8UC3);
    
    if(!server_img.isContinuous()){
         server_img = server_img.clone();
    }
    int imgSize = server_img.total() * server_img.elemSize();
    fprintf(stderr,"%d\n",imgSize);
    window[1].header.length=height;
    window[1].header.seqNumber=1;
    window[1].header.fin=0;
    window[1].header.ack=0;
    window[1].header.checksum = crc32(0L, (const Bytef *)window[1].data, 1000);
    ack_recv[1] = 2;

    window[2].header.length=width;
    window[2].header.seqNumber=2;
    window[2].header.fin=0;
    window[2].header.ack=0;
    window[2].header.checksum = crc32(0L, (const Bytef *)window[2].data, 1000);
    ack_recv[2] = 2;
    
    uchar buffer[imgSize];
    uchar *buf_ptr=buffer;
    // while(1){
    //     cap >> server_img;
        
    //     // Get the size of a frame in bytes 
        
    //     // Allocate a buffer to load the frame (there would be 2 buffers in the world of the Internet)
    //     // Copy a frame to the buffer
    //     memcpy(buffer, server_img.data, imgSize);
    //     fprintf(stderr,"%.30s\n",buffer);
    //     // send(client_sock,buffer,imgSize,0);
    //     if(server_img.empty())break;
        
    // }

    // eof,window full
    int remain=0;
    int readblock;
    bool end = false,finack=false;
    while(!finack){
        
        // if(server_img.empty())break;
        for(readblock=0;readblock<window_size&&!end;readblock++)
        {
            int write_ind = (readblock+win_start)%300;
            if(remain<=0)
            {
                cap>> server_img;
                if(server_img.empty())
                {
                    
                    remain =1000;
                    unsigned long checksum = crc32(0L, (const Bytef *)window[(readblock+win_start)%300].data, 1000);
                    window[write_ind].header.checksum = checksum;
                    window[write_ind].header.seqNumber = win_start+readblock;
                    window[write_ind].header.fin=1;
                    window[write_ind].header.ack=0;
                    ack_recv[write_ind]=3;
                    end = true;
                }
                else
                {
                    memcpy(buffer, server_img.data, imgSize);
                    remain = imgSize;
                    buf_ptr = buffer;
                }
                
            }
            if(remain<1000)bzero(window[(readblock+win_start)%300].data,1000);
            
            if(ack_recv[write_ind]==0)
            {
                memcpy(window[(readblock+win_start)%300].data,buf_ptr,MIN(remain,1000));
                buf_ptr+=MIN(remain,1000);
                unsigned long checksum = crc32(0L, (const Bytef *)window[(readblock+win_start)%300].data, 1000);
                window[write_ind].header.checksum = checksum;
                window[write_ind].header.seqNumber = win_start+readblock;
                window[write_ind].header.fin=0;
                window[write_ind].header.ack=0;
                window[write_ind].header.length = MIN(remain,1000);
                remain-=1000;
                if(remain<1000)
                {
                    window[write_ind].header.syn = 1;
                }
                else window[write_ind].header.syn = 0;
            }
            if(ack_recv[write_ind]==1)
            {
                if(window[write_ind].header.fin!=1)
                    printf("resnd\tdata\t#%d,\twinSize = %d\n",win_start+readblock,window_size);
                else
                    printf("resnd\tfin\n");
            }
            else 
            {
                if(window[write_ind].header.fin!=1)
                    printf("send\tdata\t#%d,\twinSize = %d\n",win_start+readblock,window_size);
                else
                    printf("send\tfin\n");
            }
            sendto(sendersocket, &window[write_ind], sizeof(s_tmp), 0, (struct sockaddr *)&agent, agent_size);
            
            ack_recv[write_ind]=1;
        }
        struct pollfd fd;
        int ret;
        
        fd.fd = sendersocket; // your socket handler 
        fd.events = POLLIN;
        int success=win_start;
        bool timeout = false;
        for(int i=0;i<readblock;i++)
        {
            ret = poll(&fd, 1, 1000); // 1 second for timeout
            if(ret == -1)
            {
                perror("poll");
                break;
            }
            else if(ret ==0)
            {
                //timeout
                timeout = true;
                break;
                
            }
            else
            {
                /* Receive message from receiver */
                memset(&s_tmp, 0, sizeof(s_tmp));
                segment_size = recvfrom(sendersocket, &s_tmp, sizeof(s_tmp), 0, 
                (struct sockaddr *)&tmp_addr, &tmp_size);
                
                if(s_tmp.header.fin == 1) {
                    finack = true;
                    printf("recv\tfinack\n");
                    break;
                } else {
                    index = s_tmp.header.ackNumber;
                    printf("recv\tack\t#%d\n",index);
                    if(index<win_start&&s_tmp.header.seqNumber!=win_start)
                    {
                        i--;
                    }
                    for(int i=win_start;i<=index;i++)
                    {
                        ack_recv[i%300]=0;
                    }
                    if(index>win_start-1)win_start=index+1;
                    
                }

                // recv(sendersocket,buf,sizeof(buf), 0); // get your data
            }
        }
        if(!timeout&&win_start==success+readblock)
        {
            if(window_size<threshold)window_size<<=1;
            else
            {
                window_size+=1;
            }
        }
        else
        {
            threshold = MAX(window_size/2,1);
            window_size = 1;
            if(timeout)printf("time\tout,\t\tthreshold = %d\n",threshold);
        }
    }
    cap.release();

    return 0;
}
