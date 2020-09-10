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

#define UDP_PORT_NUMBER 22452
#define AWS_UDP_PORT 23452
#define HOSTNAME "127.0.0.1"

//Variables with sockets file handlers
int svb_udp_server_fd;

//Variables with sockets addresses
struct sockaddr_in svb_udp_addr;
struct sockaddr_in aws_udp_addr;

//Variables with buffers
double rcv_buffer[25] = {0};

//sva_boot_up function
//Check the Server A booted up normally
//return 0 if success, and 1 if not
int svb_boot_up(){
	//Create the UDP socket
	if ((svb_udp_server_fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("Fail to create Server B UDP Socket.");
		return 1;
	}

	//Configure the addresses UDP of Server A 
	svb_udp_addr.sin_family = AF_INET;
    svb_udp_addr.sin_addr.s_addr = inet_addr(HOSTNAME);;
    svb_udp_addr.sin_port = htons(UDP_PORT_NUMBER);

    //Configure the addresses UDP of AWS
    aws_udp_addr.sin_family = AF_INET;
    aws_udp_addr.sin_addr.s_addr = inet_addr(HOSTNAME);;
    aws_udp_addr.sin_port = htons(AWS_UDP_PORT);

	//Bind the two sockets
	if (bind(svb_udp_server_fd, (struct sockaddr *) & svb_udp_addr, sizeof(svb_udp_addr)) < 0) 
    { 
        perror("Fail to bind Server B UDP Socket.\n"); 
        return 1;
    }

    //Print the message on the screen
    printf("The Server B is up and running using UDP on port <%d>.\n", UDP_PORT_NUMBER);
    return 0;
}

int main()
{
    
    if (svb_boot_up()){
    	perror("Server B fails to work.");
    	return 1;
    }

    while(1){
    	//Receive from AWS Server
    	int msglen, n;
    	n = recvfrom(svb_udp_server_fd, (char *)rcv_buffer, sizeof(rcv_buffer), MSG_WAITALL, (struct sockaddr *) & aws_udp_addr, &msglen);
    	int nod_num = (int)rcv_buffer[0];
        double f_size = rcv_buffer[1];
        double nod_idx[nod_num];
        double distance[nod_num];

        for (int i = 0; i < nod_num; i++){
        	nod_idx[i] = rcv_buffer[i + 2];
        	distance[i] = rcv_buffer[i + nod_num + 2];
        }

        double p_speed = rcv_buffer[2 * nod_num + 2];
        double t_speed = rcv_buffer[2 * nod_num + 3];
        int src_idx = (int)rcv_buffer[2 * nod_num + 4];

        printf("The Server B has received data for calculation:\n");
        printf("* Propagation speed: <%.2f> km/s;\n", p_speed);
        printf("* Transmission speed <%.2f> Bytes/s;\n", t_speed);
        for (int i = 0; i < nod_num; i++)
        	if(nod_idx[i] != src_idx)
    			printf("* Path length for destination <%.0f>:<%.0f>;\n", nod_idx[i], distance[i]);

        //Calculate the delays of each node
        double delay[nod_num];
        double p_delay[nod_num];
        double t_delay = f_size / (8 * t_speed);
        for (int i = 0; i < nod_num; i++){
        	p_delay[i] = distance[i] / p_speed;
        	delay[i] = p_delay[i] + t_delay;
        }

        printf("The Server B has finished the calculation of the delays:\n");
        printf("------------------------\n");
    	printf("Destination\tDelay\n");
    	printf("------------------------\n");
    	for (int i = 0; i < nod_num; i++){
    		if(nod_idx[i] != src_idx)
    			printf("%0.f\t\t%.2f\n", nod_idx[i], delay[i]);
    	}
    	printf("------------------------\n");
    	
    	//Send the results to AWS Server
    	double delay_res[nod_num + 1];
    	for (int i = 0; i < nod_num; i++)
    		delay_res[i] = p_delay[i];
    	delay_res[nod_num] = t_delay;

    	if (sendto(svb_udp_server_fd, (char *)delay_res, sizeof(delay_res),  MSG_CONFIRM, (const struct sockaddr *) &aws_udp_addr, sizeof(struct sockaddr)) < 0){
           	printf("Server B fails to send message to AWS.\n");
           	return 1;
        }
        else
           	printf("The Server B has finished sending the output to AWS.\n");
    }
	return 0;
}