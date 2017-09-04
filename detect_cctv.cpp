#include <zmq.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>


void crop_thread1()
{

	printf("[crop]\n");
	char t_string[10] = {"0_4"};// (검색 회차 _ 개수)
	
	void *context = zmq_ctx_new();
	void *responder = zmq_socket (context, ZMQ_REQ);
	int rc = zmq_bind(responder, "tcp://*:5560");

	for(int i=0 ; i<1;i++)
	{
		char buffer [40] = {0,}, sbuff[40] = {0,} ,rbuff[40]={0,};
		
		strcpy(sbuff, t_string);

		usleep(100);
		printf("filename send : [ %s ]\n", sbuff);
		zmq_send (responder, sbuff, strlen(sbuff), 0);
		zmq_recv (responder, buffer, 20, 0);
	
	}
}
int main(void)
{
	crop_thread1();
}
