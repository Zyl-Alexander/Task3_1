#define _GNU_SOURCE

#include "integral.h"
#include "net.h"

#define BEGIN -11000
#define END 11000
#define DELTA 0.000001

int main(int argc, char **argv)
{
	int res = 0;
	int sock_tcp = Create_TCP();
	if(sock_tcp == -1)
	{
		printf("Error : Create_TCP\n");
	}

	//printf("Created TCP socket\n");

	int unit_num = 0;
	Unit_task units[LISTEN_QUEUE_SIZE] = {};
	
	//printf("Start broadcasting . . .\n");

	unit_num = Get_units(units);	// find_clients
	if(unit_num == -1)
	{
		printf("Error : Get_units\n");
		close(sock_tcp);
		return 0;
	}
	if(unit_num == 0)
	{
		printf("No units found\n");
		close(sock_tcp);
		return 0;
	}

	//printf("Found : %d units\n", unit_num);

	res = Fill_units(sock_tcp, units, unit_num);
	if(res == -1)
	{
		printf("Error : Fill_units\n");
		close(sock_tcp);
		return 0;
	}

	//printf("Got all sockets from all responded units\n");
	//printf("CONNECTION ESTABLISHED\n");
	//printf("Creating tasks . . .\n");

	Create_Tasks(unit_num, units, BEGIN, END, DELTA);

	//printf("Created tasks\n");
	//printf("Sending tasks . . . \n");
	close(sock_tcp);

	res = Send_Tasks(unit_num, units);
	if(res == -1)
	{
		printf("Error : Send_Tasks\n");
		return 0;
	}

	//printf("Sent tasks\n");

	res = modify_socket_units(unit_num, units);
	if(res < 0)
	{
		printf("Error : modify_socket_units()\n");
	}

	//printf("Waiting for results . . .\n");

	res = Wait_units(unit_num, units);
	if(res < 0)
	{
		printf("Error : Wait_units()\n");
		return 0;
	}
	if(res == 0)
	{
		printf("------- FAIL : One unit is DEAD ---------\n");
		return 0;
	}

	//printf("Success : got all answers\n");
	//printf("Joining 'em\n");

	int i = 0;
	double answer = 0;
	for(i = 0; i < unit_num; ++i)
	{
		answer += units[i].answer;
		close(units[i].sock_fd);
	}

	printf("FINAL RESULT : %.3f\n", answer);
	return 0;
}
