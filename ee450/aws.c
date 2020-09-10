#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <errno.h> 
#include <string.h> 
#include <netdb.h>
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <sys/wait.h>

#define SERVER_A_PORT 21452
#define SERVER_B_PORT 22452
#define UDP_PORT_NUMBER 23452
#define TCP_PORT_NUMBER 24452
#define HOSTNAME "127.0.0.1"

//Variables with sockets file handlers
int aws_udp_server_fd;
int aws_tcp_server_fd;
int aws_tcp_child_fd;

//Variables with sockets addresses
struct sockaddr_in aws_udp_addr;
struct sockaddr_in aws_tcp_addr;
struct sockaddr_in sva_udp_addr;
struct sockaddr_in svb_udp_addr;
int udp_len = sizeof(aws_udp_addr);
int tcp_len = sizeof(aws_tcp_addr);

//Variables with status
//int data_rcvd_stat = 0;
//int svra_rcvd_stat = 0;

//Variables with buffers
double rcv_buffer[25] = {0};
double query_buffer[10] = {0};

//aws_boot_up function
//Check the AWS server booted up normally
//return 0 if success, and 1 if not
int aws_boot_up(){
	//Create the two sockets
	if ((aws_udp_server_fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("Fail to create AWS UDP Socket.");
		return 1;
	}

	if ((aws_tcp_server_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Fail to create AWS TCP Socket.");
		return 1;
	}

	//Configure the addresses of UDP and TCP of AWS
	aws_udp_addr.sin_family = AF_INET;
    aws_udp_addr.sin_addr.s_addr = inet_addr(HOSTNAME);
    aws_udp_addr.sin_port = htons(UDP_PORT_NUMBER);
    aws_tcp_addr.sin_family = AF_INET;
    aws_tcp_addr.sin_addr.s_addr = inet_addr(HOSTNAME);
    aws_tcp_addr.sin_port = htons(TCP_PORT_NUMBER);

    //Configure the addresses of UDP of Server A
    sva_udp_addr.sin_family = AF_INET;
    sva_udp_addr.sin_addr.s_addr = inet_addr(HOSTNAME);
    sva_udp_addr.sin_port = htons(SERVER_A_PORT);

    //Configure the addresses of UDP of Server B
    svb_udp_addr.sin_family = AF_INET;
    svb_udp_addr.sin_addr.s_addr = inet_addr(HOSTNAME);
    svb_udp_addr.sin_port = htons(SERVER_B_PORT);

	//Bind the two sockets
	if (bind(aws_udp_server_fd, (struct sockaddr *) & aws_udp_addr, udp_len) < 0) 
    { 
        perror("Fail to bind AWS UDP Socket."); 
        return 1;
    }
    if (bind(aws_tcp_server_fd, (struct sockaddr *) & aws_tcp_addr, tcp_len) < 0) 
    { 
        perror("Fail to bind AWS TCP Socket."); 
        return 1;
    }

    //Print the message on the screen
    printf("The AWS Server is up and running.\n");

    //10 connections are allowed max
    if (listen(aws_tcp_server_fd, 10) < 0){
    	perror("AWS Server Connections Full.\n");
    	return 1;
    } 

    //printf("Connection Set Successfully.\n");
    return 0;
}

int main(){
    int mapid = 0;
    double f_size = 0;
    double src_idx = 0;

	if (aws_boot_up()){
		perror("AWS fails to work.\n");
		return 1;
	}
	while(1){
		//Create child socket to establish connection
    	if ((aws_tcp_child_fd = accept(aws_tcp_server_fd, (struct sockaddr *) & aws_tcp_addr, (socklen_t *) & tcp_len))<0) 
    	{ 
        	perror("AWS Server Fails to Accept.\n"); 
        	return 1; 
    	}
	    //Receive data from Client
	    //Because the data is double so the whole length would be 8 * 10 = 80 Bytes
		if (read(aws_tcp_child_fd, (char *)query_buffer, 80) < 0){
	        printf("AWS fails to receive message.\n");
	        return 1;
	    }
	    else{
			//Print for debugging
			//printf("%0.f, %0.f, %0.f\n", query_buffer[0], query_buffer[1], query_buffer[2]);
	        mapid = (int)query_buffer[0];
	        src_idx = query_buffer[1];
	        f_size = query_buffer[2];
	        printf("The AWS has recieved map ID <%c>, start vertex <%0.f> and file size <%0.f> from the client using TCP over port <%d>.\n", mapid, src_idx, f_size, TCP_PORT_NUMBER);
		}

	    //Send data to ServerA
	    double data_to_A[5] = {0};
	    data_to_A[0] = (double)mapid;
	    data_to_A[1] = src_idx;
	    if (sendto(aws_udp_server_fd, (char *)data_to_A, sizeof(data_to_A),  MSG_CONFIRM, (const struct sockaddr *) &sva_udp_addr, sizeof(struct sockaddr)) < 0){
	       	printf("AWS fails to send message to Server A.\n");
	       	return 1;
	    }
	    else
	       	printf("The AWS has sent map ID and starting vertex to server A using UDP over port <%d>”\n", UDP_PORT_NUMBER);

	    //Receive the results from server A
	    int msglen, n;
		n = recvfrom(aws_udp_server_fd, (char *)rcv_buffer, sizeof(rcv_buffer), MSG_WAITALL, (struct sockaddr *) & sva_udp_addr, &msglen);
		int nod_num = (int)rcv_buffer[0]; // Get the # of nodes

		double nod_idx[nod_num];
		double distance[nod_num];
		double speed[2];
		for (int i = 0; i < nod_num; i++){
			nod_idx[i] = rcv_buffer[i+1];
			distance[i] = rcv_buffer[i + nod_num + 1];
		}
		speed[0] = rcv_buffer[2 * nod_num + 1];
		speed[1] = rcv_buffer[2 * nod_num + 2];
		//Print the result that received
		printf("The AWS has received shortest path from server A:\n");
		printf("-----------------------------\n");
		printf("Destination\tMin Length\n");
		printf("-----------------------------\n");
		for (int i = 0; i < nod_num; i++){
			if(nod_idx[i] != src_idx){
				printf("%.0f\t\t%.0f\n", nod_idx[i], distance[i]);
			}
		}
		printf("-----------------------------\n");

		//Send data to Server B
		double data_to_B[2 * nod_num + 5];
		data_to_B[0] = nod_num;
		data_to_B[1] = f_size;
		for (int i = 0; i < nod_num; i++){
			data_to_B[i + 2] = nod_idx[i];
			data_to_B[i + nod_num + 2] = distance[i];
		}
		data_to_B[2 * nod_num + 2] = speed[0];
		data_to_B[2 * nod_num + 3] = speed[1];
		data_to_B[2 * nod_num + 4] = src_idx;
		if (sendto(aws_udp_server_fd, (char *)data_to_B, sizeof(data_to_B),  MSG_CONFIRM, (const struct sockaddr *) &svb_udp_addr, sizeof(struct sockaddr)) < 0){
	       	printf("AWS fails to send message to Server B.\n");
	       	return 1;
	    }
	    else
	       	printf("The AWS has sent path length, propagation speed and transmission speed to server B using UDP over port <%d>”\n", UDP_PORT_NUMBER);

	    //Receive the results from server B
		n = recvfrom(aws_udp_server_fd, (char *)rcv_buffer, sizeof(rcv_buffer), MSG_WAITALL, (struct sockaddr *) & svb_udp_addr, &msglen);
		double t_delay = rcv_buffer[nod_num];
		double p_delay[nod_num];
		for (int i = 0; i < nod_num; i++)
			p_delay[i] = rcv_buffer[i];
		
		//Print the result that received
		printf("The AWS has received delays from server B:\n");
		printf("--------------------------------------------\n");
		printf("Destination\tTt\tTp\tDelay\n");
		printf("--------------------------------------------\n");
		for (int i = 0; i < nod_num; i++)
			if(nod_idx[i] != src_idx)
				printf("%.0f\t\t%.2f\t%.2f\t%.2f\n", nod_idx[i], t_delay, p_delay[i], t_delay+p_delay[i]);
		printf("--------------------------------------------\n");

		//Send the result to Client
		double data_to_C[3 * nod_num + 2];
		data_to_C[0] = nod_num;
		for (int i = 0; i < nod_num; i++){
			data_to_C[i+1] = nod_idx[i];
			data_to_C[i + nod_num + 1] = p_delay[i];
			data_to_C[i + 2 * nod_num + 1] = distance[i];
		}
		data_to_C[3 * nod_num + 1] = t_delay; 

		if (send(aws_tcp_child_fd, (char *)data_to_C, sizeof(data_to_C), 0) < 0){
			perror("AWS fails to send message to Client.\n");
			return 1;
		}
		else{
			printf("The AWS has sent calculated delay to client using TCP over port <%d>.\n", TCP_PORT_NUMBER);
			close(aws_tcp_child_fd);
		}
	}
	return 0;
}
