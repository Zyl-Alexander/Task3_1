#define _GNU_SOURCE

//#include "integral.h"
#include "net.h"



int Input_handler(int argc, char ** argv)
{
	if(argc != 2)
	{
		printf("Error: number of arguments\n");
		exit(0);
	}
	char *endp;
	int a;

	a = strtol(argv[1], &endp, 10);
	if(errno == ERANGE)
	{
		printf("Error : get number\n");	exit(0);
	}
	if(*endp != '\0')
	{
		printf("Error : get number\n");	exit(0);
	}
	if(a <= 0)
	{
		printf("Error : got invalid argument <= 0\n");	exit(0);
	}
	return a;
}


int udp_connection(struct sockaddr_in* server_addr)
{
	assert(server_addr);

	int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock_fd < 0)
	{
		printf("Error : socket()\n");
		return -1;
	}
	
	struct sockaddr_in broadcast_addr;
	broadcast_addr.sin_family	=	AF_INET;
	broadcast_addr.sin_port		=	htons(UDP_PORT);
	broadcast_addr.sin_addr.s_addr	=	htonl(INADDR_ANY);


	if(bind(sock_fd, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr)) == -1)
	{
		printf("Error : bind()\n");
		close(sock_fd);
		return -1;
	}

	int len = sizeof(server_addr);
	char c;

	printf("Start listening . . . \n");

	int recv = recvfrom(sock_fd, &c, sizeof(c), 0, (struct sockaddr*)server_addr, &len);
	if(recv < 0)
	{
		printf("Error : recvfrom()\n");
		close(sock_fd);
		return -1;
	}


	int optval = 1;

	if(setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) == -1)
	{
		printf("Error : setsockopt()\n");
		close(sock_fd);
		return -1;
	}


	char back_msg = '1';
	int i = 0;
	for(i = 0; i < 5; ++i)
	{
		int cnt = sendto(sock_fd, &back_msg, sizeof(back_msg), 0, (struct sockaddr*)server_addr, sizeof(*server_addr));
		if(cnt < 0)
		{
			printf("Error : sendto - udp_connetion\n");
			close(sock_fd);
			return -1;
		}
	}
	//printf("Send broadcast msg back to server\n");

	close(sock_fd);
	return 1;
}



int Create_TCP()
{
	int sock_tcp = socket(AF_INET, SOCK_NONBLOCK | SOCK_STREAM, 0);
	if(sock_tcp < 0)
	{
		printf("Error : socket() - tcp\n");
		return -1;
	}

	int optval = 1;
	if(setsockopt(sock_tcp, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
	{
		printf("Error : setsockopt() - tcp\n");
		close(sock_tcp);
		return -1;
	}

	struct sockaddr_in serv_addr;
	serv_addr.sin_family	=	AF_INET;
	serv_addr.sin_port		=	htons(TCP_PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(sock_tcp, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
	{
		printf("Error : bind() - tcp\n");
		close(sock_tcp);
		return -1;
	}

	//max number of computers - LISTEN_QUEUE_SIZE = 30
	if(listen(sock_tcp, LISTEN_QUEUE_SIZE) == -1)
	{
		printf("Error : listen() - tcp\n");
	}

	return sock_tcp;
}



int update_units(struct sockaddr_in* unit_addr, Unit_task units[LISTEN_QUEUE_SIZE], int unit_num)
{
	int i = 0;
	for(i = 0; i < LISTEN_QUEUE_SIZE; ++i)
	{
		if(units[i].addr == unit_addr->sin_addr.s_addr)
		{
			//printf("In update_units : found the same\n");
			return unit_num;
		}
		else
		{
			if(units[i].addr == 0)
			{
				//printf("Got new unit #%d\n", unit_num);
				units[i].addr = unit_addr->sin_addr.s_addr;
				return (unit_num + 1);
			}
		}
	}
}



int Select_units(int sock_fd, Unit_task units[LISTEN_QUEUE_SIZE])
{
	struct sockaddr_in unit_addr;
	int unit_num = 0;
	int queue_size = 0;
	int nfds = sock_fd + 1;
	int addr_len = sizeof(unit_addr);

	do
	{
		struct timeval timeout;
		timeout.tv_sec = 0; timeout.tv_usec = 120000;
		fd_set socket_set;
		FD_ZERO(&socket_set);
		FD_SET(sock_fd, &socket_set);

		queue_size = select(nfds, &socket_set, NULL, NULL, &timeout);

		if(queue_size < 0)
		{
			printf("Error : select() in Select_units\n");
			return -1;
		}
		else
		{
			if(queue_size == 0)
			{
				//printf("queue_size == 0 - Select_units\n");
				return unit_num;
			}
			else
			{
				char msg;
				int res = recvfrom(sock_fd, &msg, sizeof(msg), 0, (struct sockaddr*)&unit_addr, &addr_len);
				if(res < 0)
				{
					printf("Error : recfrom() in Select_units\n");
					return -1;
				}
				//printf("Recieved new msg from unit\n");
				unit_num = update_units(&unit_addr, units, unit_num);
			}
		}


	}while(queue_size);

	return unit_num;
}



int Get_units(Unit_task units[LISTEN_QUEUE_SIZE])
{
	int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock_fd < 0)
	{
		printf("Error : socket()\n");
		return -1;
	}

	int optval = 1;

	//prepare socket to BROADCAST
	if(setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) == -1)
	{
		printf("Error : setsockopt()\n");
		close(sock_fd);
		return -1;
	}

	//broadcast to all local network
	struct sockaddr_in broadcast_addr;
	broadcast_addr.sin_family	=	AF_INET;
	broadcast_addr.sin_port		=	htons(UDP_PORT);
	inet_pton(AF_INET, "255.255.255.255", &(broadcast_addr.sin_addr));

	int unit_num = 0;	
	int iter = 0;
	int len = 0;

	do
	{

		char msg = '1';
		int i = 0;
		for(i = 0; i < 10; ++i)
		{
			int cnt = sendto(sock_fd, &msg, sizeof(msg), 0, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
			if(cnt < 0)
			{
				printf("Error : sendto() - Get_units\n");
				error_t error_num = errno;
				char *errorbuf = strerror(error_num);
        		fprintf(stderr, "Error message : %s\n", errorbuf);
				close(sock_fd);
				return -1;
			}
		}

		unit_num = Select_units(sock_fd, units);
		if(unit_num < 0)
		{
			printf("Error : Select_units\n");
		}

		iter++;
	}while(!unit_num && iter < 5);

	close(sock_fd);
	return unit_num;
}


int tcp_connection(struct sockaddr_in* serv_addr)
{
	int sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_tcp < 0)
	{
		printf("Error : socket() in tcp_connection\n");
		return -1;
	}

	// sets PORT of addr structure of server, recieved via broadcast
	serv_addr->sin_port = htons(TCP_PORT);
	int res = connect(sock_tcp, (struct sockaddr*)serv_addr, sizeof(*serv_addr));
	if(res == -1)
	{
		printf("Error : connect() in tcp_connection\n");
		close(sock_tcp);
		return -1;
	}
	return sock_tcp;
}

int Fill_units(int sock_tcp, Unit_task units[LISTEN_QUEUE_SIZE], int unit_num)
{
	int respond_num = 0;
	int queue_size = 0;
	int iter = 0;
	int nfds = sock_tcp + 1;
	do
	{
		struct timeval timeout;
		timeout.tv_sec = 0; timeout.tv_usec = 120000;

		fd_set socket_set;
		FD_ZERO(&socket_set);
		FD_SET(sock_tcp, &socket_set);

		queue_size = select(nfds, &socket_set, NULL, NULL, &timeout);
		if(queue_size < 0)
		{
			printf("Error : select() in Fill_units\n");
			return -1;
		}
		else
		{
			if(queue_size == 0)
			{
				iter++;
			}
			else
			{
				int sock_unit = accept(sock_tcp, NULL, NULL);
				if(sock_unit < 0)
				{
					printf("Error : accept() in Fill_units\n");
					return -1;
				}

				int logs_unit = 0;
				int recv_len = recv(sock_unit, &(logs_unit), sizeof(logs_unit), 0);
				if(recv_len < 0)
				{
					printf("Error : recv() in Fill_units\n");
					return -1;
				}
				if(recv_len == 0)
				{
					printf("Failed to get logs from unit\n");
					return -1;
				}
				units[respond_num].sock_fd	=	sock_unit;
				units[respond_num].logs 	=	logs_unit;
				respond_num++;
				//printf("Got info from unit #%d!!!\n", respond_num - 1);
			}
		}

	}while(iter < 4 && respond_num < unit_num);

	if(respond_num < unit_num)
	{
		printf("Failed to get all units who responded to broadcast\n");
		return -1;
	}
	return 1;
}


int Send_to_server(int sock_tcp, int logs)
{
	char logs_char = (char) logs;
	int res = send(sock_tcp, &logs_char, sizeof(logs_char), 0);
	if(res < 0)
	{
		printf("Error : send() in Send_to_server\n");
		return -1;
	}
	return 1;
}


void Create_Tasks(int unit_num, Unit_task units[LISTEN_QUEUE_SIZE], double begin, double end, double delta)
{
	int sum_logs = 0; int i = 0;
	for(i = 0; i < unit_num; ++i)
	{
		sum_logs += units[i].logs;
	}
	//printf("All number of threads : %d\n", sum_logs);

	double step = (end - begin) / sum_logs;
	double now = begin;
	for(i = 0; i < unit_num; ++i)
	{
		units[i].begin = now;
		now += step * units[i].logs;
		units[i].end = now;
		units[i].delta = delta;
	}
}

ssize_t send_info(int sock_tcp, void* msg, size_t size, int flags)
{
	ssize_t len_now = 0;
	ssize_t len_full = 0;

	do
	{
		len_now = send(sock_tcp, (char*)msg + len_full, size - len_full, flags);
		if(len_now < 0)
		{
			printf("Error : send() in send_info\n");
			return -1;
		}
		len_full += len_now;

	}while(len_now && len_full < size);

	return len_full;
}

ssize_t recv_info(int sock_tcp, void* msg, size_t size, int flags)
{
	ssize_t len_now = 0;
	ssize_t len_full = 0;

	do
	{
		len_now = recv(sock_tcp, (char*)msg + len_full, size - len_full, flags);
		if(len_now < 0)
		{
			printf("Error : recv() in recv_info\n");
			return -1;
		}
		len_full += len_now;

	}while(len_now && len_full < size);

	return len_full;
}

int Send_Tasks(int unit_num, Unit_task units[LISTEN_QUEUE_SIZE])
{
	size_t size = sizeof(units[0].begin);
	int i = 0; int res = 0;

	for(i = 0; i < unit_num; ++i)
	{
		res = send_info(units[i].sock_fd, &(units[i].begin), size, 0);
		if(res < 0)
		{
			printf("Error : send_info() in Send_Tasks\n");
			return -1;
		}
		else
		{
			if(res < size)
			{
				printf("Error : not all chars sent to unit - Send Tasks\n");
				return -1;
			}
		}
		
		res = send_info(units[i].sock_fd, &(units[i].end), size, 0);
		if(res < 0)
		{
			printf("Error : send_info() in Send_Tasks\n");
			return -1;
		}
		else
		{
			if(res < size)
			{
				printf("Error : not all chars sent to unit - Send Tasks\n");
				return -1;
			}
		}
		
		
		res = send_info(units[i].sock_fd, &(units[i].delta), size, 0);
		if(res < 0)
		{
			printf("Error : send_info() in Send_Tasks\n");
			return -1;
		}
		else
		{
			if(res < size)
			{
				printf("Error : not all chars sent to unit - Send Tasks\n");
				return -1;
			}
		}
	}

	return 1;
}

int Get_from_server(int sock_tcp, double *d)
{
	ssize_t len = recv_info(sock_tcp, d, sizeof(double), 0);

	if(len < 0)
	{
		printf("Error : recv_info() - Get_from_server\n");
		return -1;
	}
	else
	{
		if(len < sizeof(double))
		{
			printf("Error : received less than double - Get_from_server\n");
			return -1;
		}
	}

	return 1;
}

/*
TCP_KEEPIDLE (начиная с Linux 2.4) - Время (в секундах) простоя (idle)
соединения, по прошествии которого TCP начнёт отправлять проверочные пакеты
(keepalive probes), если для сокета включён параметр SO_KEEPALIVE.
Данный параметр не должен использоваться, если требуется переносимость кода.

TCP_KEEPINTVL (начиная с Linux 2.4)
Время в секундах между отправками отдельных проверочных пакетов 
(keepalive probes). Данный параметр не должен использоваться, если 
требуется переносимость кода.

TCP_KEEPCNT (начиная с Linux 2.4)
Максимальное число проверок (keepalive probes) TCP, отправляемых 
перед сбросом соединения. Данный параметр не должен использоваться, 
если требуется переносимость кода.
*/

int save_socket(int sock_tcp)
{
	int res = 0;
	int option = 1;
	res = setsockopt(sock_tcp, SOL_SOCKET, SO_KEEPALIVE, &option, sizeof(option));
	if(res)
	{
		printf("Error : setsockopt() - SO_KEEPALIVE - save_socket\n");
		return -1;
	}

	option = 5;
	res = setsockopt(sock_tcp, IPPROTO_TCP, TCP_KEEPIDLE, &option, sizeof(option));
	if(res)
	{
		printf("Error : setsockopt() - TCP_KEEPIDLE - save_socket\n");
		return -1;
	}

	option = 2;
	res = setsockopt(sock_tcp, IPPROTO_TCP, TCP_KEEPCNT, &option, sizeof(option));
	if(res)
	{
		printf("Error : setsockopt() - TCP_KEEPCNT - save_socket\n");
		return -1;
	}

	option = 1;
	res = setsockopt(sock_tcp, IPPROTO_TCP, TCP_KEEPINTVL, &option, sizeof(option));
	if(res)
	{
		printf("Error : setsockopt() - TCP_KEEPINTVL - save_socket\n");
		return -1;
	}
	return 1;
}

static void* monitor_server(void* arg)
{
	int sock_tcp = *((int*)arg);
	int tmp = 0;
	recv(sock_tcp, &tmp, sizeof(tmp), 0);
	printf("Someone is dead in the network;\n");
	printf("Calculations Failed\n");
	printf("-------------- END OF FAILED SESSION ------------\n");
	exit(EXIT_FAILURE);
}


int modify_socket_server(int* psock_tcp)
{
	int res = save_socket(*psock_tcp);
	pthread_t self_thread;

	if(res < 0)
	{
		printf("Error : save_socket - modify_socket\n");
		return -1;
	}

	if(pthread_create(&self_thread, NULL, monitor_server, psock_tcp))
	{
		printf("Error : pthread_create() - modify_socket\n");
		return -1;
	}
	return 1;
}

int modify_socket_units(int unit_num, Unit_task units[LISTEN_QUEUE_SIZE])
{
	int res = 0;
	int i = 0;
	for(i = 0 ; i < unit_num; ++i)
	{
		res = save_socket(units[i].sock_fd);
		if(res < 0)
		{
			printf("Error : save_socket() - modify_socket_units\n");
			return -1;
		}
	}
	return 1;
}


int Wait_units(int unit_num, Unit_task units[LISTEN_QUEUE_SIZE])
{
	int queue_size = 0; int i = 0;
	
	while(1)
	{
		int max_fd = -1;
		for(i = 0; i < unit_num; ++i)
		{
			if(!units[i].status && units[i].sock_fd > max_fd)
			{
				max_fd = units[i].sock_fd;
			}
		}

		if(max_fd == -1)
		{
			break; // collected all results
		}

		int nfds = max_fd + 1;

		struct timeval timeout;
		timeout.tv_sec = 5; timeout.tv_usec = 0;

		fd_set socket_set;
		FD_ZERO(&socket_set);
		for(i = 0; i < unit_num; ++i)
		{
			if(!units[i].status)
			{
				FD_SET(units[i].sock_fd, &socket_set);
			}
		}

		queue_size = select(nfds, &socket_set, NULL, NULL, &timeout);

		if(queue_size < 0)
		{
			printf("Error : select() - Wait_units\n");
			return -1;
		}
		
		if(queue_size == 0)
		{
			//printf("All continue working\n");
		}
		else
		
		for(i = 0; i < unit_num; ++i)
		{
			if(FD_ISSET(units[i].sock_fd, &socket_set))
			{
				units[i].status = 1;

				double answer = 0;
				int res = recv_info(units[i].sock_fd, &answer, sizeof(answer), 0);
				if(res < 0)
				{
					printf("Error : recv_info - Wait_units\n");
					return -1;
				}
				if(res == 0) // connection was killed
				{
					return 0;
				}
				if(res < sizeof(answer))
				{
					printf("Error: size of answer recved - Wait_units\n");
					return -1;
				}

				units[i].answer = answer;
			}
		}
	}
	return 1;
}

