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

#define UDP_PORT_NUMBER 21452
#define AWS_UDP_PORT 23452
#define MAX_ND_NUM 10
#define INFINITY 999
#define HOSTNAME "127.0.0.1"

//Variables with sockets file handlers
int sva_udp_server_fd;

//Variables with sockets addresses
struct sockaddr_in sva_udp_addr;
struct sockaddr_in aws_udp_addr;

//Variables with buffers
double rcv_buffer[10] = {0};

//sva_boot_up function
//Check the Server A booted up normally
//return 0 if success, and 1 if not
int sva_boot_up(){
	//Create the UDP socket
	if ((sva_udp_server_fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("Fail to create Server A UDP Socket.");
		return 1;
	}

	//Configure the addresses UDP of Server A 
	sva_udp_addr.sin_family = AF_INET;
    sva_udp_addr.sin_addr.s_addr = inet_addr(HOSTNAME);;
    sva_udp_addr.sin_port = htons(UDP_PORT_NUMBER);

    //Configure the addresses UDP of AWS
    aws_udp_addr.sin_family = AF_INET;
    aws_udp_addr.sin_addr.s_addr = inet_addr(HOSTNAME);;
    aws_udp_addr.sin_port = htons(AWS_UDP_PORT);

	//Bind the two sockets
	if (bind(sva_udp_server_fd, (struct sockaddr *) & sva_udp_addr, sizeof(sva_udp_addr)) < 0) 
    { 
        perror("Fail to bind Server A UDP Socket.\n"); 
        return 1;
    }

    //Print the message on the screen
    printf("The Server A is up and running using UDP on port <%d>.\n", UDP_PORT_NUMBER);
    return 0;
}

//To find the index of certain element in an int array
int idx_find(double array[], int len, double element){
	int i = 0;
	for (i = 0; i < len; i++){
		if (element == array[i])
			return i;
	}
	return -1;
}

//The Dijkstra algorithm
/*####Some Codes and Ideas in this Method is From 'GeeksforGeeks'####*/
int Dijkstra(double matrix[MAX_ND_NUM][MAX_ND_NUM], double nod_idx[MAX_ND_NUM], int n, double source, double distance[]){
	int pred[MAX_ND_NUM], visited[MAX_ND_NUM];
	int count, mindistance, nextnode;
	int idx = idx_find(nod_idx, n, source);

	//initialize pred[],distance[] and visited[]
	for(int i = 0;i < n; i++){
		distance[i] = matrix[idx][i];
		pred[i] = idx;
		visited[i] = 0;
	}

	distance[idx] = 0;
	visited[idx] = 1;
	count = 1;

	while(count < n - 1){
		mindistance = INFINITY;
		
		//nextnode gives the node at minimum distance
		for(int i = 0; i < n; i++)
			if(distance[i] < mindistance && !visited[i]){
				mindistance = distance[i];
				nextnode = i;
			}
			
		//check if a better path exists through nextnode			
		visited[nextnode] = 1;
		for(int i = 0; i < n; i++)
			if(!visited[i])
				if(mindistance + matrix[nextnode][i] < distance[i]){
					distance[i] = mindistance + matrix[nextnode][i];
					pred[i] = nextnode;
				}
		count++;
	}

	//print the path and distance of each node
	printf("The Server A has identified the following shortest paths:\n");
	printf("-----------------------------\n");
	printf("Destination\tMin Length\n");
	printf("-----------------------------\n");
	for(int i = 0; i < n; i++)
		if(i != idx){
			printf("%.0f\t\t%.0f\n", nod_idx[i], distance[i]);
		}
	printf("-----------------------------\n");
	return 0;
}

//Open the specific map file
FILE* open_file(char file_name[]){
	FILE* map = fopen(file_name, "r");
	if (map == NULL){
		printf("The Map file dosen't exist.\n");
		return NULL;
	}
	return map;
}

//To remove the duplicates in an array return the qunique numbers;
int rmv_dupl(double array[], int len, double target[]){
	int count = 0;
	int index = 0;
	for (int i = 0; i < len; i++){
    	for (index = 0; index < count; index++){
      		if(array[i] == target[index])
        	break;
    	}
    	if (index == count){
      		target[count] = array[i];
      		count++;
    	} 
	}
	return count;
}

//Compare Function to do the array
int compare (const void * e1, const void * e2) 
{
    int a = *((double*)e1);
    int b = *((double*)e2);
    if (a > b) return  1;
    if (a < b) return -1;
    return 0;
}

//Find the length of the file
int f_size_find(FILE* map){
	int f_size = 0;
	fseek(map, 0L, SEEK_END);
	f_size = ftell(map);
	rewind(map);
	return f_size;
}

//Copy the whole file
void copy_file(FILE* map, char f_ctnt[], int f_size){
	for (int i = 0; !feof(map); i++){
		fscanf(map, "%c", &f_ctnt[i]);
	}
	f_ctnt[f_size] = '\0';
}

//Replace the '\n' with ' '
void replace(char f_ctnt[], int f_size){
	for (int i = 0; i < f_size; i++){
		if (f_ctnt[i] == '\n')
			f_ctnt[i] = ' ';
	}
}

//Split a string with interval '' store in target
int convert(char input[], double target[]){
	char *p = input;
	int i = 0;
	while(p){
    	p = strtok(p, " ");
    	if (p[0] >= 'A' && p[0] <= 'Z')
    		target[i] = -p[0];
    	else
    		target[i] = atof(p);
    	//printf("%d\n", res[i]);
    	p = strtok(NULL, "=");
    	i++;
    }
    return i;
}

//Count the # of maps
int count_map(double f_ctnt_d[], int new_size){
	int num = 0;
	for (int i = 0; i < new_size; i++){
		if (f_ctnt_d[i] < 0)
			num++;
	}
	return num;
}

//Find the starts of each map
void start_find(int new_size, double f_ctnt_d[], char map_title[], int map_pos[]){
	int idx = 0;
	for (int i = 0; i < new_size; i++){
		if (f_ctnt_d[i] < 0){
			map_title[idx] = (char)-f_ctnt_d[i];
			map_pos[idx] = i;
			idx++;
		}
	}
}

//Calculate the numbers of edges
void calc_edge(int map_num, int new_size, int edg_num[], int map_pos[]){
	for (int i = 0; i < map_num; i++){
		if (i != map_num-1)
			edg_num[i] = (map_pos[i+1] - map_pos[i]) / 3 - 1;
		else
			edg_num[i] = (new_size - map_pos[i]) / 3 - 1;
	}
}

//Count the # of verices
void count_vertice(int new_size, int map_num, int map_pos[], double f_ctnt_d[], int nod_num[], double nod_idx[][MAX_ND_NUM], double speed[][2]){
	for (int i = 0; i < map_num; i++){
		int start = 0; 
		int end = new_size;
		double nd_buff1[new_size];
		double nd_buff2[new_size];
		for (int j = 0; j < new_size; j++){
			nd_buff1[j] = -1;
			nd_buff2[j] = -1;
		}
		if (i != 0)
			start = map_pos[i];
		if (i != map_num-1)
			end = map_pos[i+1];
		for (int j = start; j < end; j++){
			if (j - start == 1)
				speed[i][0] = f_ctnt_d[j];
			if (j - start == 2)
				speed[i][1] = f_ctnt_d[j];
			if (((j-start) % 3) != 2 && (j - start) > 2)
				nd_buff1[j] = f_ctnt_d[j];
		}
		//Get the number of nodes
		nod_num[i] = rmv_dupl(nd_buff1, new_size, nd_buff2) - 1;
		//Get the indexes of each node stored in nod_idx[][]
		double nd_idx_buff[nod_num[i]];
		//Initialize the nod_idx[i][]
		for (int j = 0; j < MAX_ND_NUM; j++)
			nod_idx[i][j] = -1;
		//Transport form nd_buff2 to nd_idx_buff
		for (int j = 0; j < nod_num[i]; j++)
			nd_idx_buff[j] = nd_buff2[j+1];
		//Transport from nd_idx_buf to nod_idx
		for (int j = 0; j < nod_num[i]; j++)
			nod_idx[i][j] = nd_idx_buff[j];
		qsort(nod_idx[i], nod_num[i], sizeof(double), compare);
	}
}

//Print the info of each map
void print_info(int map_num, char map_title[], int nod_num[], int edg_num[]){
	printf("-------------------------------------------\n");
	printf("Map ID\tNum Vertices\tNum Edges\t\n");
	printf("-------------------------------------------\n");
	for (int i = 0; i < map_num; i++){
		printf("%c\t%d\t\t%d\t\t\t\n", map_title[i], nod_num[i], edg_num[i]);
	}
	printf("-------------------------------------------\n");
}

//Initialize the graph matrix
void initi_matrix(int map_num, double graph_matrix[][MAX_ND_NUM][MAX_ND_NUM]){
	for (int i = 0; i < map_num; i++)
		for (int j = 0; j < MAX_ND_NUM; j++)
			for (int k = 0; k < MAX_ND_NUM; k++)
				graph_matrix[i][j][k] = INFINITY;
}

//Construct the graph matrix
void build_matrix(int map_num, int new_size, int map_pos[], int nod_num[], double nod_idx[][MAX_ND_NUM], double f_ctnt_d[], double graph_matrix[][MAX_ND_NUM][MAX_ND_NUM]){
	for (int i = 0; i < map_num; i++){
		int start = 0; 
		int end = new_size;
		if (i != 0)
			start = map_pos[i];
		if (i != map_num-1)
			end = map_pos[i+1];
		for (int j = start + 3; j < end; j += 3){
			int src = idx_find(nod_idx[i], nod_num[i], f_ctnt_d[j]);
			int dst = idx_find(nod_idx[i], nod_num[i], f_ctnt_d[j+1]);;
			double value = f_ctnt_d[j+2];
			graph_matrix[i][src][dst] = value;
			graph_matrix[i][dst][src] = value;
		}
	}
}

//Write the sub-map into smaller files
void write_matrix(int map_num, char map_title[], double graph_matrix[][MAX_ND_NUM][MAX_ND_NUM], double nod_idx[][MAX_ND_NUM], int nod_num[], double speed[][2]){
	char graph_name[] = "graph_.txt";
	for (int i = 0; i < map_num; i++){
		graph_name[5] = map_title[i];
		FILE* fptr = fopen(graph_name, "w");
		for (int j = 0; j < MAX_ND_NUM; j++)
			for (int k = 0; k < MAX_ND_NUM; k++)
				fprintf(fptr, "%.0f ", graph_matrix[i][j][k]);
		for (int j = 0; j < MAX_ND_NUM; j++){
			fprintf(fptr, "%.0f ", nod_idx[i][j]);
		}
		fprintf(fptr, "%d ", nod_num[i]);
		fprintf(fptr, "%f ", speed[i][0]);
		fprintf(fptr, "%f ", speed[i][1]);
		fclose(fptr);
	}
}

//Construct the Map Matrix
int map_construction(){
	
	//Open the File return if not exists
	FILE* map = open_file("map.txt");

	//Find the length of the file
	int new_size = 0; // integer num
	int map_num = 0; // map num
	int f_size = f_size_find(map); // character num
	char f_ctnt[f_size + 1];// character file
	double f_ctnt_d[f_size];// double file
	

	//Copy the whole file into Array f_ctnt
	copy_file(map, f_ctnt, f_size);

	//Replace the '\n' with ' '
	replace(f_ctnt, f_size);

	//Convert into int[] stored in f_ctnt_int[]
	new_size = convert(f_ctnt, f_ctnt_d);
	
	//Count the numbers of maps
	map_num = count_map(f_ctnt_d, new_size);

	//Find the starts of each map and store the title
	int map_pos[map_num];
	char map_title[map_num];
	start_find(new_size, f_ctnt_d, map_title, map_pos);

	//Calculate the numbers of edges
	int edg_num[map_num];
	calc_edge(map_num, new_size, edg_num, map_pos);

	//Count the numbers of vertices
	int nod_num[map_num];
	double nod_idx[map_num][MAX_ND_NUM];
	double speed[map_num][2];
	count_vertice(new_size, map_num, map_pos, f_ctnt_d, nod_num, nod_idx, speed);
	
	//Print the info of each map
	print_info(map_num, map_title, nod_num, edg_num);

	//Construct the Graph Matrix
	double graph_matrix[map_num][MAX_ND_NUM][MAX_ND_NUM];
	//Initialize the graph matrix
	initi_matrix(map_num, graph_matrix);
	//Construct the graph matrix
	build_matrix(map_num, new_size, map_pos, nod_num, nod_idx, f_ctnt_d, graph_matrix);

	//Write the graph_matrix into files
	write_matrix(map_num, map_title, graph_matrix, nod_idx, nod_num, speed);

	//int check = 2;
	//Dijkstra(graph_matrix[check], nod_idx[check], nod_num[check], 6);
	return 0;
}

int retrieve_matrix(int mapid, double graph_matrix[MAX_ND_NUM][MAX_ND_NUM], double nod_idx[MAX_ND_NUM], double speed[2]){

	char graph_name[] = "graph_.txt";
	graph_name[5] = (char)mapid;
	FILE* graph = open_file(graph_name);
	int f_size = f_size_find(graph);
	char g_ctnt[f_size + 1];
	double g_ctnt_d[MAX_ND_NUM * (MAX_ND_NUM + 1) + 3];
	//Copy File to Array
	copy_file(graph, g_ctnt, f_size);
	convert(g_ctnt, g_ctnt_d);

	for (int i = 0; i < MAX_ND_NUM; i++)
		for (int j = 0; j < MAX_ND_NUM; j++)
			graph_matrix[i][j] = g_ctnt_d[i * MAX_ND_NUM + j];
	for (int i = 0; i < MAX_ND_NUM; i++)
		nod_idx[i] = g_ctnt_d[i + MAX_ND_NUM * MAX_ND_NUM];
	int nod_num = (int)g_ctnt_d[MAX_ND_NUM * (MAX_ND_NUM + 1)];
	speed[0] = g_ctnt_d[MAX_ND_NUM * (MAX_ND_NUM + 1) + 1];
	speed[1] = g_ctnt_d[MAX_ND_NUM * (MAX_ND_NUM + 1) + 2];
	return nod_num;
}

int main(){

	int nod_num = 0;
	double speed[2] = {0};
	double nod_idx[MAX_ND_NUM] = {0};
	double distance[MAX_ND_NUM] = {0};
	double graph_matrix[MAX_ND_NUM][MAX_ND_NUM];

	if (sva_boot_up()){
		perror("Server A fails to work.\n");
		return 1;
	}

	//Construct the map and save
	map_construction();

	while(1){
		//Receive from AWS Server
		int msglen, n;
		n = recvfrom(sva_udp_server_fd, (char *)rcv_buffer, sizeof(rcv_buffer), MSG_WAITALL, (struct sockaddr *) & aws_udp_addr, &msglen);
		int mapid = (char)rcv_buffer[0];
	    double src_idx = rcv_buffer[1];
	    printf("The Server A has received input for finding shortest paths: starting vertex <%.0f> of map <%c>.\n", src_idx, mapid);
	    
	    //Retreive the graph matrix from graph file
	    nod_num = retrieve_matrix(mapid, graph_matrix, nod_idx, speed);

	    //Compute the shortest path with Dijkstra algorithm
	    Dijkstra(graph_matrix, nod_idx, nod_num, src_idx, distance);

	   	//Store the message to send in one array
	    double path_res[2 * nod_num + 3];
	    path_res[0] = nod_num;
	    for (int i = 0; i < nod_num; i++){
	    	path_res[i+1] = nod_idx[i];
	    	path_res[i + nod_num + 1] = distance[i];
	    }
	    path_res[2 * nod_num + 1] = speed[0];
	    path_res[2 * nod_num + 2] = speed[1];

	    //Send the path info to AWS
	    if (sendto(sva_udp_server_fd, path_res, sizeof(path_res),  MSG_CONFIRM, (const struct sockaddr *) & aws_udp_addr, sizeof(struct sockaddr)) < 0){
	    	printf("Server A fails to send message to AWS.\n");
	    	return 1;
	    }
	    else
	    	printf("The Server A has sent shortest paths to AWS.\n");
	}
	return 0;
}