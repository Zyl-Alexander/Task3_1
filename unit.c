#define _GNU_SOURCE

#include "integral.h"
#include "net.h"


int main(int argc, char **argv)
{
	int n = Input_handler(argc, argv);
	
	//Establish connection
	struct sockaddr_in serv_addr;
	//printf("Looking for broadcast msg . . . \n");
	int res = udp_connection(&serv_addr);
	if(res == -1)
	{
		printf("Error : udp_connection\n");
		return 0;
	}

	printf("--------------------- START OF SESSION --------------\n");
	//printf("Responded to broadcast\n");
	//printf("Trying to establish TCP connection . . . \n");
	
	int sock_tcp = tcp_connection(&serv_addr); //estavblish_main_connection
	if(sock_tcp == -1)
	{
		printf("Error : tcp_connection\n");
		return 0;
	}

	//printf("TCP connection established\n");
	//printf("Sending info to server\n");

	int logs = Log_num();

	if(n > logs)
	{
		n = logs;
	}

	res = Send_to_server(sock_tcp, n);
	if(res == -1)
	{
		printf("Error : Send_to_server\n");
		return 0;
	}

	//printf("Send logs to server\n");
	//printf("Waiting for task from server . . . \n");

	double begin = 0, end = 0, delta = 0;
//-------------------------------------------------
	res = Get_from_server(sock_tcp, &begin);
	if(res < 0)
	{
		printf("Error : Get_from_server (begin)\n");
		return 0;
	}

	res = Get_from_server(sock_tcp, &end);
	if(res < 0)
	{
		printf("Error : Get_from_server (end)\n");
		return 0;
	}

	res = Get_from_server(sock_tcp, &delta);
	if(res < 0)
	{
		printf("Error : Get_from_server (end)\n");
		return 0;
	}
//------------------------------------------------
	//printf("Got all tasks from server\n");
	//printf("Begin = %f; End = %f; Delta = %f\n", begin, end, delta);

	res = modify_socket_server(&sock_tcp);
	if(res < 0)
	{
		printf("Error : modify_socket()\n");
		return 0;
	}

	//printf("CALCULATION . . .\n");
	double answer = Main_Integral(n, begin, end, delta);

	//printf("CALCULATION finished\n");
	//printf("Sending answer to server . . .\n");

	res = send_info(sock_tcp, &answer, sizeof(answer), 0);
	if(res < 0)
	{
		printf("Error : send_info() - sending answer\n");
		return 0;
	}

	//printf("Send answer to server\n");
	printf("-------------------- END OF SESSION --------------------\n");

	return 0;
}


