#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <mysql.h>

#define PORT 9002
#define IPADDR "192.168.1.49"

#define BUFSIZE 1024

#define ARG_MAX 6
const char *host = (char*)"localhost";
const char *user = (char*)"root";
const char *pw = (char*)"bitiotansehen";
const char *db = (char*)"ansehen";
char    buffer[BUFSIZ];
int max_cctv;


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

main( )
{
	int	c_socket, s_socket;
	struct sockaddr_in c_addr, s_addr;
	int	len;
	int	n;

	MYSQL *connection;
        MYSQL_RES  *sql_result;
        MYSQL_ROW sql_row;

        char query[BUFSIZ];
	char *ptr;
	char c_buff[ARG_MAX][BUFSIZ];
	char	rcvBuffer[BUFSIZ];
	
	char cctv_id[5];
	char ip_add[20];

	c_socket = socket(PF_INET, SOCK_STREAM, 0);
	
	memset(&c_addr, 0, sizeof(c_addr));
	c_addr.sin_addr.s_addr = inet_addr(IPADDR);
	c_addr.sin_family = AF_INET;
	c_addr.sin_port = htons(PORT);
	
	if(connect(c_socket, (struct sockaddr *) &c_addr, sizeof(c_addr))==-1) {
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

// send 넣어주기

	send(c_socket, cctv_id, strlen(cctv_id)+1, 0);
	send(c_socket, ip_add, strlen(ip_add)+1, 0);
	printf("ip_address : %s\n",ip_add);

///////////////////////

	/// unique key
	
	if((n = read(c_socket, rcvBuffer, sizeof(rcvBuffer))) < 0) {
		return (-1);
	}
	
	rcvBuffer[n] = '\0';
	printf("received Data : %s\n", rcvBuffer);

// file ( while 문 안에 넣어 주기 )


	// 파일 이름 받기
	char filename[256+1];
	int retval = recv(c_socket, filename, 256, MSG_WAITALL);
	if(retval < 0){
		err_display("recv()");
		close(c_socket);
//		continue;
	}
	filename[retval] = '\0';
	printf("-> 받을 파일 이름: %s\n", filename);

	// 파일 검색
	int currbytes = 0;
	if(search_file(filename)){
		// 현재의 파일 크기 얻기
		FILE *fp = fopen(filename, "rb");
		if(fp == NULL){
			perror("파일 입출력 오류");
			close(c_socket);
	//		continue;
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
	//	continue;
	}

	// 전송 받을 데이터 크기 받기
	int totalbytes;
	retval = recv(c_socket, (char *)&totalbytes, sizeof(totalbytes), MSG_WAITALL);
	if(retval < 0){
		err_display("recv()");
		close(c_socket);
	//	continue;
	}
		printf("-> 전송 받을 데이터 크기: %d 바이트 중 %d 바이트\n"
		       "                         (%d 바이트는 이미 받음)\n",
			currbytes+totalbytes, totalbytes, currbytes);

		// 파일 열기
		FILE *fp = fopen(filename, "ab"); // append & binary mode
		if(fp == NULL){
			perror("파일 입출력 오류");
			close(c_socket);
	//		continue;
		}

		// 파일 데이터 받기
		int numtotal = 0;
		while(1){
			retval = recv(c_socket, buf, BUFSIZE, MSG_WAITALL);
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

		// 전송 결과 출력
		if(numtotal == totalbytes)
			printf("-> 파일 전송 완료!\n");
		else
			printf("-> 파일 전송 실패!\n");







	
	close(c_socket);
}
