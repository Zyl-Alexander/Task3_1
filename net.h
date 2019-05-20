#ifndef NET_H
#define NET_H

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#define LISTEN_QUEUE_SIZE 30	// N_CLIENTS_MAX
#define UDP_PORT 4027
#define TCP_PORT 4028

typedef struct unit_task
{
	uint32_t addr;
	int sock_fd;
	int logs;
	int status;
	double begin;
	double end;
	double delta;
	double answer;
} Unit_task, *pUnit_task;


//-------------  SERVER  -----------------

int Create_TCP();
// creates TCP socket, bind it to TCP_PORT, start listening on it
// return sock_tcp, or -1 if failed

int Get_units(Unit_task units[LISTEN_QUEUE_SIZE]);
// broadcasts and get addresses in units structure
//return unit_num if OK, -1 if not

int Select_units(int sock_fd, Unit_task units[LISTEN_QUEUE_SIZE]); 
// waits for returning broadcast msg from units, counts 'em', sets units' address
//return unit_num if OK, -1 if not

int Fill_units(int sock_tcp, Unit_task units[LISTEN_QUEUE_SIZE], int unit_num);
//accepts tcp connections from units; gets their num of logs
// returns 1 if OK, -1 if not

void Create_Tasks(int unit_num, Unit_task units[LISTEN_QUEUE_SIZE], double begin, double end, double delta);
//creates tasks for all units due to their log num

ssize_t send_info(int sock_tcp, void* msg, size_t size_msg, int flags);
//sends msg to sock_tcp (sends limits to units)
//return number of sended bytes, or -1 if fail

ssize_t recv_info(int sock_tcp, void* msg, size_t size_msg, int flags);
//recv msg from sock_tcp (gets limits for integral)
//return number of received bytes, or -1 if fail

int Send_Tasks(int unit_num, Unit_task units[LISTEN_QUEUE_SIZE]);
//Send all created tasks to all connected units
// returns 1 if OK, -1 if not

int modify_socket_units(int unit_num, Unit_task units[LISTEN_QUEUE_SIZE]);
// acts save_socket to all sock_fd in units
// returns 1 if OK, -1 if not

int Wait_units(int unit_num, Unit_task units[LISTEN_QUEUE_SIZE]);
//waits and monitors all units, fill units struct when they finish with answers
// returns 1 if OK, 0 if connection was killed, -1 if error

//------------  UNIT  ------------------

int Input_handler(int argc, char ** argv);
//operates with input
//returns number in input of exits

int udp_connection(struct sockaddr_in *serv_addr);
// gets broadcast msg from server and responds it's ready
// returns 1 if OK, -1 if not

int tcp_connection(struct sockaddr_in* serv_addr);
// sends request to server for TCP connection
// returns connected socket to server or -1

int Send_to_server(int sock_tcp, int n);
//send num of availible logs to server
// returns 1 if OK, -1 if not

int Get_from_server(int sock_tcp, double *d);
//recv d from sock_tcp (for limits of integral)
// returns 1 if OK, -1 if not

int save_socket(int sock_tcp); 
// sets keepalive probes and their timeouts
// returns 1 if OK, -1 if not

int modify_socket_server(int* psock_tcp);
// acts save_socket to sock_tcp and sets monitoring thread for server connection
// returns 1 if OK, -1 if not



#endif 
