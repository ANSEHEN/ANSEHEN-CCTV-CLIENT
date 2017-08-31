#include <zmq.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>

class Data{
	public:
	char unique_key[100];
	char image_addr[200];
};
void crop_thread1()
{

	printf("[crop]\n");
	char t_string[2][10] = {"0_4","1_2"};// (검색 회차 _ 개수)
	
	Data data;
	
	void *context = zmq_ctx_new();
	void *responder = zmq_socket (context, ZMQ_REQ);
	int rc = zmq_bind(responder, "tcp://*:5560");

	for(int i=0 ; i<1;i++)
	{
		char buffer [40] = {0,}, sbuff[40] = {0,} ,rbuff[40]={0,};
		strcpy(data.image_addr, t_string[i]);
		strcpy(rbuff, data.image_addr);
		printf("rbuff: %s\n", rbuff);
		usleep(100);
		snprintf(sbuff, sizeof(sbuff), "%s", rbuff);
		printf("filename send : [ %s ]\n", sbuff);
		zmq_send (responder, sbuff, strlen(sbuff), 0);
		zmq_recv (responder, buffer, 20, 0);
		printf(" %s\n",buffer);
	}
}
void crop_thread2()
{

	printf("[crop]\n");
	char t_string[2][10] = {"0_4","1_2"};
	
	Data data;
	
	void *context = zmq_ctx_new();
	void *responder = zmq_socket (context, ZMQ_REQ);
	int rc = zmq_bind(responder, "tcp://*:5560");

	for(int i=0; i<2;i++)
	{
		char buffer [40] = {0,}, sbuff[40] = {0,} ,rbuff[40]={0,};
		strcpy(data.image_addr, t_string[i]);
		strcpy(rbuff, data.image_addr);
		printf("rbuff: %s\n", rbuff);
		usleep(100);
		snprintf(sbuff, sizeof(sbuff), "%s", rbuff);
		printf("filename send : [ %s ]\n", sbuff);
		zmq_send (responder, sbuff, strlen(sbuff), 0);
		zmq_recv (responder, buffer, 20, 0);
		printf(" %s\n",buffer);
	}
}
int main(void)
{
	crop_thread2();
}
