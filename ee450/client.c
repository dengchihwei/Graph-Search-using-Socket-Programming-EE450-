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

#define TCP_PORT_NUMBER 24452
#define HOSTNAME "127.0.0.1"

//Variables with sockets
int tcp_client_fd;
int client_port_number = 0;
struct sockaddr_in aws_addr;
struct sockaddr_in client_addr;

//Variables with status
int sent_stat = 0;

//Variables with buffers
double rcv_buffer[35] = {0};

//client_boot_up function
//Check the client booted up normally
//return 0 if success, and 1 if not
int client_boot_up(){
	//Create the TCP socket
	if ((tcp_client_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Fail to create Client TCP Socket.\n");
		return 1;
	}

	aws_addr.sin_family = AF_INET;
	aws_addr.sin_addr.s_addr = inet_addr(HOSTNAME);
    aws_addr.sin_port = htons(TCP_PORT_NUMBER); 

	if(connect(tcp_client_fd,(struct sockaddr *) & aws_addr, sizeof(struct sockaddr)) < 0)
    {
        perror("Fail to connect to AWS.\n");
        return 1;
    }

    socklen_t len = sizeof(struct sockaddr);
 	getsockname(tcp_client_fd, (struct sockaddr *) & client_addr, &len);
 	client_port_number = client_addr.sin_port;

    //Print the message on the screen
    printf("The Client is up and running.\n");
    return 0;
}


int main(int argc, char *argv[]){

	if (argc != 4){
		printf("Input is Wrong!\n");
		return 1;
	}

	if (client_boot_up()){
		perror("Client fails to work.\n");
		return 1;
	}

	double mapid = (double) argv[1][0];
	double src_idx = atof(argv[2]);
	double f_size = atof(argv[3]);

	double msg[10] = {0};
	msg[0] = mapid;
	msg[1] = src_idx;
	msg[2] = f_size;

	if (send(tcp_client_fd, (char *)msg, sizeof(msg), 0) < 0){
		sent_stat = 0;
		perror("Client fails to send message.\n");
		return 1;
	}
	else{
		sent_stat = 1;
		printf("The client has sent query to AWS using TCP over port <%d>: start vertex <%0.f>; map <%c>; file size <%0.f>.\n", client_port_number, src_idx, (char)mapid, f_size);
	}

	//Receive data from Server
    //Because the data is double so the whole length would be 8 * 25 = 200 Bytes
	if (read(tcp_client_fd, (char *)rcv_buffer, 280) < 0){
        printf("Client fails to receive message.\n");
        return 1;
    }
    else{
    	int nod_num = (int)rcv_buffer[0];
        double nod_idx[nod_num];
        double p_delay[nod_num];
        double distance[nod_num];
        double t_delay = rcv_buffer[3 * nod_num + 1];
        for (int i = 0; i < nod_num; i++){
        	nod_idx[i] = rcv_buffer[i + 1];
        	p_delay[i] = rcv_buffer[i + nod_num + 1];
        	distance[i] = rcv_buffer[i + 2 * nod_num + 1];
        }
        printf("The client has received results from AWS:\n");
        printf("-------------------------------------------------------\n");
        printf("Destination\tMin Length\tTt\tTp\tDelay\n");
        printf("-------------------------------------------------------\n");
        for (int i = 0; i < nod_num; i++)
        	if (nod_idx[i] != src_idx )
        		printf("%.0f\t\t%.2f\t\t%.2f\t%.2f\t%.2f\n", nod_idx[i], distance[i], t_delay, p_delay[i], t_delay + p_delay[i]);
        printf("-------------------------------------------------------\n");
	}

	return 0;
}