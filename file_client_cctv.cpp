#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <mysql.h>
#include <iostream>

#include <sys/ipc.h>
#include <sys/msg.h>

//추가
#include <zmq.h>
#include <assert.h>
#include <sys/types.h>

#include <thread>

using namespace std;

class Data{
	public: 
	char unique_key[100];
	char image_addr[200];
};
class Cctv_data
{
	public: 
		char cctv_id [5];
		char ip [20];
};
class mbuf {
	public :
	long mtype;
	char buf[100];
	char unique_key[100];
	char image_addr[200];
};

#define PORT 9002
#define IPADDR "192.168.1.49"
#define BUFSIZE 1024
#define ARG_MAX 6

const int type_beacon = 1;
const int type_snd = 2;
const int type_rcv = 3;
const int type_run = 4;
const int type_out = 5;
const char *host = (char*)"localhost";
const char *user = (char*)"root";
const char *pw = (char*)"bitiotansehen";
const char *db = (char*)"ansehen";
char    buffer[BUFSIZ];

int max_cctv;
int msgid ;

int c_socket;


// 소켓 함수 오류 출력 후 종료
void err_quit(const char *msg)
{
	perror(msg);
	exit(1);
}

// 소켓 함수 오류 출력
void err_display(const char *msg)
{
	perror(msg);
}

// 현재 디렉터리에서 파일 존재 여부를 확인한다.
int search_file(const char *filename)
{
	return (access(filename, F_OK) == 0);
}
void dataToCCTV(char *unique_key, char * filename);
void SendMsgToCCTV(char *unique_key, char * filename);

void send_message(void *t)
{
	mbuf msg;
	void *context = zmq_ctx_new();
	void *responder = zmq_socket (context, ZMQ_REQ);
	int rc = zmq_bind(responder, "tcp://*:5550");

	while(1)
	{
		msgrcv(msgid, (void*)&msg, sizeof(mbuf), type_snd ,0);
		char buffer [100] = {0,};

		strcpy(buffer, msg.image_addr);
		printf("rbuff: %s\n", buffer);
		usleep(100);

		printf("filename send : [ %s ]\n", buffer);
		zmq_send (responder, buffer, strlen(buffer), 0);

		zmq_recv (responder, buffer, 20, 0);
	}
}

void recv_message(void *t)
{
	//mbuf msg;

	void *context = zmq_ctx_new();
	void *responder = zmq_socket (context, ZMQ_REQ);
	int rc = zmq_bind(responder, "tcp://*:5570");
	MYSQL *connection;
        MYSQL_RES  *sql_result;
        MYSQL_ROW sql_row;
	char query[BUFSIZ];
	char result;

	mbuf detect_msg;
	detect_msg.mtype = type_out;

	while(1)
	{
		//msgrcv(msgid, (void*)&msg, sizeof(mbuf), type_rcv ,0);
		char buffer [100] = {0,} ;

		strcpy(buffer, "recv_ready");
		zmq_send (responder, buffer, strlen(buffer), 0);
		zmq_recv (responder, buffer, sizeof(buffer), 0);
		if (strlen(buffer) > 0)
		{
			printf("매치 결과 : %s\n", buffer);
			
			//db로 자료 올리기(USER_INFO_CCTV테이블에 result에 넣기 1= match 0=mismatch)
		
			connection = mysql_init(NULL);
		        if(!mysql_real_connect(connection,host,user,pw,db,0,NULL,0))
		        {
		                fprintf(stderr,"%s\n",mysql_error(connection));
				exit(1);
		        }
			
		        //send sql query
			
			sprintf(query,"update USER_INFO_CCTV set result = 1 where image_add ='%s'",buffer);
		        if(mysql_query(connection,query))
		        {
		                fprintf(stderr,"%s\n",mysql_error(connection));
		                exit(1);	
		        }
			msgsnd(msgid, (void*)&detect_msg, sizeof(mbuf), 0);//detect 동작 중지
			printf("buffer : %s\n", buffer);
			mysql_close(connection);
		}
	}
	
}

main( )
{
	
	struct sockaddr_in c_addr;
	int	len;
	int	n;
        char query[BUFSIZ];
	char *ptr;
	char c_buff[ARG_MAX][BUFSIZ];
	char cctv_id[5];
	char ip_add[20];

	MYSQL *connection;
        MYSQL_RES  *sql_result;
        MYSQL_ROW sql_row;
	Data data;
	msgid = msgget(1234, IPC_CREAT);

	//socket connect
	c_socket = socket(AF_INET, SOCK_STREAM, 0);
	
	memset(&c_addr, 0, sizeof(c_addr));
	c_addr.sin_addr.s_addr = inet_addr(IPADDR);
	c_addr.sin_family = AF_INET;
	c_addr.sin_port = htons(PORT);
	
	if(connect(c_socket, (struct sockaddr *) &c_addr, sizeof(c_addr))<0) {
		printf("Can not connect\n");
	  	close(c_socket);
		return -1;
	}
	socklen_t addrlen;
	char buf[BUFSIZE];


//mysql 에서 cctv 정보 읽어오기

	connection = mysql_init(NULL);
        if(!mysql_real_connect(connection,host,user,pw,db,0,NULL,0))
        {
                fprintf(stderr,"%s\n",mysql_error(connection));
		exit(1);
        }

        //send sql query
        if(mysql_query(connection, "select * from CCTV_INFO"))
        {
                fprintf(stderr,"%s\n",mysql_error(connection));
                exit(1);
        }
        sql_result = mysql_use_result(connection);

        //output table name
        printf("MySQL Tables in mysql database:\n");
        while((sql_row=mysql_fetch_row(sql_result))!=NULL)
        {        
		printf("cctv_id : %s\n",sql_row[0]);
		strcpy(cctv_id, sql_row[0]);
		printf("ip_address : %s\n",sql_row[2]);
		strcpy(ip_add, sql_row[2]);
	}
        mysql_free_result(sql_result);

	
	// cctv 정보 server에 보내주기 (send 넣어주기)
	
	Cctv_data cctv_data;
	strcpy(cctv_data.cctv_id,cctv_id);
	strcpy(cctv_data.ip,ip_add);

	send(c_socket, &cctv_data, sizeof(cctv_data), 0);
	//send(c_socket, ip_add, strlen(ip_add)+1, 0);
	printf("[send] cctv_id : %s \nip_address : %s\n",cctv_data.cctv_id, cctv_data.ip);

// 파이썬과 통신하는 쓰레드 생성
	int a = 0;
	thread t1(send_message, (void*)&a);
	thread t2(recv_message, (void*)&a);

// file 받는 부분 ( while 문 안에 넣어 주기 )

	while(1)
	{
		int state;
		recv(c_socket, &state, sizeof(state),MSG_WAITALL);
		//printf("TT : %d\n",TT);
		
		if(state==1)
		{
			//class로 data 받기
			Data data;
			char unique_key[100];
			char filename[256];
			
			bzero(&data, sizeof(data));
	
			int retval = recv(c_socket, &data, sizeof(data),MSG_WAITALL);
			if(retval < 0){
				err_display("recv()");
				close(c_socket);
				continue;
			}
			else{
				printf("got data\n");
			}
			strcpy(filename, data.image_addr);
			strcpy(unique_key, data.unique_key);
		
			// 파일 검색
			int currbytes = 0;
			if(search_file(filename)){
				// 현재의 파일 크기 얻기
				FILE *fp = fopen(filename, "rb");
				if(fp == NULL){
					perror("파일 입출력 오류");
					close(c_socket);
					continue;
				}
				fseek(fp, 0, SEEK_END);
				currbytes = ftell(fp);
				fclose(fp);
			}
	
			// 전송을 시작할 위치(=현재의 파일 크기) 보내기 (이어받기)
			retval = send(c_socket, (char *)&currbytes, sizeof(currbytes), 0);
			if(retval < 0){
				err_display("send()");
				close(c_socket);
				continue;
			}
	
			// 전송 받을 데이터 크기 받기
			int totalbytes;
			retval = recv(c_socket, (char *)&totalbytes, sizeof(totalbytes), 0);
			if(retval < 0){
				err_display("recv()");
				close(c_socket);
				continue;
			}
			printf("-> 전송 받을 데이터 크기: %d 바이트 중 %d 바이트\n"
				       "                         (%d 바이트는 이미 받음)\n",
					currbytes+totalbytes, totalbytes, currbytes);
	
			// 파일 열기
			FILE *fp = fopen(filename, "ab"); // append & binary mode
			if(fp == NULL){
				perror("파일 입출력 오류");
				close(c_socket);
				continue;
			}
	
			// 파일 데이터 받기
			int numtotal = 0;
			while(1){
				if(numtotal == totalbytes){
					break;
				}
				retval = recv(c_socket, buf, BUFSIZE, 0);
				printf("retval : %d\n",retval);
				if(retval > 0){	
					fwrite(buf, 1, retval, fp);
					if(ferror(fp)){
						perror("파일 입출력 오류");
						break;
					}
				numtotal += retval;
				}
				else if(retval == 0){
					break;
				}
				else{
					err_display("recv()");
					break;
				}
			}
			fclose(fp);
		


//intercommunication between python and c++ using zmq    

			// 전송 결과 출력
			if(numtotal == totalbytes)
			{
				printf("-> 파일 전송 완료!\n");
				mbuf msg;
				msg.mtype = type_snd;
				strcpy(msg.image_addr, filename);
				msgsnd(msgid, (void*)&msg, sizeof(mbuf), 0);
 				
				dataToCCTV(unique_key, filename);
				
			}
			else
			{
				printf("-> 파일 전송 실패!\n");
			}

		}
		else if(state == 2) //비컨 영역에  유저가 들어왔을 때
		{	
			mbuf detect_msg;
			detect_msg.mtype = type_run;
			
			msgsnd(msgid, (void*)&detect_msg, sizeof(mbuf), 0);
			printf("유저 등장\n");
			


			char unique_key[100];

			int retval = recv(c_socket, unique_key, sizeof(unique_key),0);
			printf("unique_key_beacon signal : %s\n",unique_key);
			
			//SendMsgToCCTV(unique_key, filename);
			//face detect 부분에 비컨신호 알려주는 함수 : 매개변수 변경해야함.
		}
		else if (state == 3) //비컨영역에서 벗어났을 때
		{
			mbuf detect_msg;
			detect_msg.mtype = type_out;
			
			char query[BUFSIZ];
			int result;
			char unique_key[100];

			int retval = recv(c_socket, unique_key, sizeof(unique_key),0);
			printf("unique_key_beacon signal : %s\n",unique_key);
		
		        //send sql query
			
			sprintf(query,"select result from USER_INFO_CCTV where unique_key ='%s'",unique_key);
		        if(mysql_query(connection,query))
		        {
		                fprintf(stderr,"%s\n",mysql_error(connection));
		                exit(1);
		        }
			sql_result = mysql_use_result(connection);
     			sql_row=mysql_fetch_row(sql_result);
        		result =atoi(sql_row[0]);
			mysql_free_result(sql_result);

			//result = 0(stay), 1(match), 2(fail when target out)
			if(result != 1)
			{
				sprintf(query,"update USER_INFO_CCTV set result = 2 where unique_key ='%s'",unique_key);
		        
				if(mysql_query(connection,query))
			        {
			                fprintf(stderr,"%s\n",mysql_error(connection));
			                exit(1);
				}
				result = 2;
				printf("result is not 1.\n");
				msgsnd(msgid, (void*)&detect_msg, sizeof(mbuf), 0);//detect 동작 중지
				
		        }
		
			printf("result : %d\n", result);

			
			send(c_socket, &result, sizeof(result), 0);
		}
		else
			printf("state error!!!");
	}
	close(c_socket);
	mysql_close(connection);
}
//DB에    unique key, image_add(filename)저장 하기
void dataToCCTV(char *unique_key, char * image_add)
{
	MYSQL *connection;
        MYSQL_RES  *sql_result;
        MYSQL_ROW sql_row;
	char query[BUFSIZ];

	
	connection = mysql_init(NULL);
        if(!mysql_real_connect(connection,host,user,pw,db,0,NULL,0))
        {
                fprintf(stderr,"%s\n",mysql_error(connection));
		exit(1);
        }

        //send sql query
	
	sprintf(query,"insert into USER_INFO_CCTV (unique_key,image_add,result) values ('%s', '%s',0)",unique_key,image_add);
        if(mysql_query(connection,query))
        {
                fprintf(stderr,"%s\n",mysql_error(connection));
                exit(1);
        }
      
	printf("unique_key : %s\n", unique_key);
	printf("image_add : %s\n", image_add);
	mysql_close(connection);
}

//비컨 신호가 왔을때 cctv detect 동작하라고 신호 보내기 !
void SendMsgToCCTV(char *t_unique_key, char * t_image_add)
{
	mbuf msg;
	msg.mtype = type_beacon;
	strcpy(msg.unique_key,t_unique_key);
	strcpy(msg.image_addr, t_image_add);
	msgsnd(msgid, (void*)&msg, sizeof(mbuf), 0);
}
