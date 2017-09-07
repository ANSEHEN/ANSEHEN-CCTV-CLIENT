#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <thread>
#include <iostream>
#include <mutex>

#include <zmq.h>
#include <string.h>
#include <assert.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>

using namespace std;
using namespace cv;

mutex f_mtx;
int msgid= msgget(1234, IPC_CREAT);
int detect_msgid = msgget(4321, IPC_CREAT);

int total_manager;
bool tf=false;

const int type_snd = 9;
class mbuf{
	public :
	long mtype;
	char buf[100];	//안씀
	char unique_key[100];	//안씀
	char image_addr[200];	//안씀
};
class detect_mbuf{
	public:
	long mtype;
	char buf[100];
};

class FaceManager{
 public:
	int try_num;
	int compare_count;	//비교 얼굴 총 갯수
	FaceManager():try_num(0),compare_count(0){}
	void AddTryNum(){
		try_num++;
	}
	void AddCompareCount(){
		compare_count++;
	}
	int GetCompareCount(){
		return compare_count;
	}
	int GetTryNum(){
		return try_num;
	}
	void CompareFaceInit(){
		int i;
		cout<<"compare_face_data Delete"<<endl;
		f_mtx.lock();
		cout<<"실행 완료"<<endl;
		compare_count=0;
		f_mtx.unlock();
	}
	/*
	~FaceManager(){
		int i;
		for(i=0;i<compare_count;i++){
			delete face_compare;
		}
	}
	*/
/*
	void CompareFace(){	//thread 생성해서 작동
		int i,j;
		for(i=0;i<data_count;i++){
			for(j=0;j<compare_count;j++){
				bool tf=true; //얼굴 비교 확인용 (참으로 가정)
				//
				//외부 솔루션을 통한 얼굴 비교
				//
				if(tf){	//i,j가 같은 얼굴이라고 들어온 경우
					//socket을 통하여 face_data[i] 데이터 전송 이때도 mutex 걸어야함
					RemoveData(face_data[i]);
				}
			}
		}
		CompareFaceInit();
		compare_count=0;
	}
*/
};
class TimeManagement{
 private:
	clock_t start;
	clock_t end;
 public:
	void TimeStartReset(){
		start=0;
	}
	void TimeStart(){
		start=clock();
	}
	int TimeEnd(){
		if(start!=0){
			int time;
			end=clock();
			time=((int)(end-start)/1000000);
			cout<<"Time: "<<time<<endl;
			return time;
		}
		return 0;
	}
};
void BeaconSignalReceive(int* temp){
	/*
	socket을 통해서 큰 보드에서 사진경로와 키값 데이터를 받는 소스를 구현
	*/
	mbuf msg;
	cout<<"beacon signal receive thread create"<<endl;
	while(1){
		msgrcv(msgid, (void*)&msg, sizeof(mbuf), 4, 0);
		f_mtx.lock();
		*temp=*temp+1;
		f_mtx.unlock();
		cout<<"total_manager ++"<<endl;
	}
}
void BeaconDisconnectReceive(int* temp){
	mbuf msg;
	cout<<"beacon disconnect signal receive thread create"<<endl;
	while(1){
		msgrcv(msgid, (void*)&msg, sizeof(mbuf), 5,0);
		f_mtx.lock();
		*temp=*temp-1;
		if(*temp==0){
			tf=false;
		}
		f_mtx.unlock();
		cout<<"total_manager --"<<endl;
	}
}
void KairosCommunication(FaceManager* fm){ //타이머 종료, 일정 사진이 찍힌경우
	cout<<"[Kairos Create]"<<endl;
	void *context = zmq_ctx_new();
	void *responder = zmq_socket (context, ZMQ_REQ);
	int rc = zmq_bind(responder, "tcp://*:5560");
	while(1){
		//if(tf){
			//rcv mbuffer 0_20
			
			cout<<"[crop]"<<endl;
			detect_mbuf d_buff;
			msgrcv(detect_msgid, (void*)&d_buff, sizeof(d_buff), type_snd, 0);

			//char t_string[10];
			//sprintf(t_string,"%d_%d",fm->GetTryNum(),fm->GetCompareCount()-1);
			char buffer [100] = {0,};		
			strcpy(buffer, d_buff.buf);

			usleep(100);
			cout<<"filename send : "<<mbuffer<<endl;
			zmq_send (responder, buffer, strlen(buffer), 0);
			zmq_recv (responder, buffer, sizeof(buffer), 0);
			
			//f_mtx.lock();
			//tf=false;
			//f_mtx.unlock();
		//}
	}
}
#define CAM_WIDTH 480
#define CAM_HEIGHT 300

FaceManager* fm=new FaceManager;
TimeManagement timer;


int main()
{
    detect_mbuf d_buff;
    bool af=true;
    VideoCapture cap(0);
	cap.set(CV_CAP_PROP_FRAME_WIDTH, CAM_WIDTH);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, CAM_HEIGHT);
	
    if(!cap.isOpened()){
        cerr << "Can't Open Camera" << endl;
        return -1;
    }

    namedWindow("Face", 1);

    //cascadeclassifier 클랙스
    CascadeClassifier face_classifier;

    //얼굴 인식 xml 로딩
    thread beaconConnect(&BeaconSignalReceive,&total_manager);
    thread beaconDisconnect(&BeaconDisconnectReceive,&total_manager);
    thread faceComparison(&KairosCommunication,fm);
    face_classifier.load("/home/pi/opencv_src/opencv/data/haarcascades/haarcascade_frontalface_default.xml");
    while(1){
        Mat frame_original;
        Mat frame;
		Mat face_image;
        try{
            //카메라로부터 이미지 얻어오기
            cap >> frame_original;
        }catch(Exception& e){
            cerr << "Execption occurred." << endl;
        }
        if(1){
            try{
                Mat grayframe;
                //gray scale로 변환
                cvtColor(frame_original, grayframe, CV_BGR2GRAY);
                //histogram 얻기
                equalizeHist(grayframe, grayframe);

                //이미지 표시용 변수
                vector<Rect> faces;

                //얼굴의 위치와 영역을 탐색한다.

//detectMultiScale(const Mat& image, vector<Rect>& objects, double scaleFactor=1.1,
//              int minNeighbors=3, int flags=0, Size minSize=Size(), Size maxSize=Size())
//image 실제 이미지
//objects 얼굴 검출 위치와 영역 변수
//scaleFactor 이미지 스케일
//minNeighbors 얼굴 검출 후보들의 갯수
//flags 이전 cascade와 동일하다 cvHaarDetectObjects 함수 에서
//      새로운 cascade에서는 사용하지 않는다.
//minSize 가능한 최소 객체 사이즈
//maxSize 가능한 최대 객체 사이즈
                /*face_classifier.detectMultiScale(grayframe, faces,1.1,3,CV_HAAR_FIND_BIGGEST_OBJECT | CV_HAAR_SCALE_IMAGE,Size(30, 30));*/
                face_classifier.detectMultiScale(grayframe, faces, 1.1, 3, 0, Size(30, 30));
		if(af){
			af=false;
			cout<<"얼굴 인식 전"<<endl;
		}
		if(timer.TimeEnd()>10){
			timer.TimeStartReset();
			cout<<"Time out"<<endl;
			d_buff.mtype = type_snd;
			sprintf(d_buff.buf,"%d_%d",fm->GetTryNum(),fm->GetCompareCount()-1);
			msgsnd(detect_msgid, (void*)&d_buff, sizeof(d_buff), 0);
			
			fm->AddTryNum();
			fm->CompareFaceInit();
			
			
			cout<<"[timer out]compare start!!"<<endl;
			/*
			f_mtx.lock();
			tf=true;
			f_mtx.unlock();
			*/
		}
	    	if(total_manager>0){
	            for(int i=0;i<faces.size();i++){
	                Point lb(faces[i].x + faces[i].width, faces[i].y + faces[i].height);
	                Point tr(faces[i].x, faces[i].y);
	//rectangle(Mat& img, Point pt1, Point pt2, const Scalar& color, int thickness=1, int lineType=8, int shift=0)
	//img 적용할 이미지
	//pt1 그릴 상자의 꼭지점
	//pt2 pt1의 반대편 꼭지점
	//color 상자의 색상
	//thickness 상자의 라인들의 두께 음수 또는 CV_FILLED를 주면 상자를 채운다.
	//lineType 라인의 모양 line()함수 확인하기
	//shift ?? Number of fractional bits in the point coordinates.
	//포인트 좌표의 분수 비트의 수??
			cout<<"얼굴 확인 됨"<<endl;
			af=true;
			timer.TimeStart();
			char savefile[100];
			cap>>frame;
			cap>>face_image;
			tr.x=tr.x-(lb.x-tr.x)*(0.2);
			tr.y=tr.y-(lb.y-tr.y)*(0.3);
			lb.x=lb.x+(lb.x-tr.x)*(0.2);
			lb.y=lb.y+(lb.y-tr.y)*(0.3);
			Rect rect(tr.x,tr.y,lb.x-tr.x,lb.y-tr.y);
			face_image=face_image(rect);
			imshow("image",face_image);
			int try_num=fm->GetTryNum();
			int compare_face_num=fm->GetCompareCount();
			sprintf(savefile,"%d_%d.jpg",try_num,compare_face_num);
			fm->AddCompareCount();
			char compare_path[]="/home/pi/ansehen/";
			strcat(compare_path,savefile);
			cout<<savefile<<endl;	
			imwrite(savefile,face_image);
			imshow("CCTV",frame);
			if(compare_face_num>19){
				//snd buffer<- 0_20 1_20
				
				d_buff.mtype = type_snd;
				sprintf(d_buff.buf,"%d_%d",fm->GetTryNum(),fm->GetCompareCount()-1);
				msgsnd(detect_msgid, (void*)&d_buff, sizeof(d_buff), 0);

				fm->AddTryNum();
				fm->CompareFaceInit();
				timer.TimeStartReset();
				cout<<"[count out]compare start!!"<<endl;
				/*
				f_mtx.lock();
				tf=true;
				f_mtx.unlock();
				*/
			}
			//sprintf(savefile,"image %d_%d.jpg",face_num,count_num++);
			//imwrite(savefile,frame);
			rectangle(frame_original, lb, tr, Scalar(0, 255, 0), 3, 4, 0);
			waitKey(500);
		}
            }
//윈도우에 이미지 그리기
        imshow("Face", frame_original);
        }catch(Exception& e){
			cerr << "Exception occurred. face" << endl;
        }
//키 입력 대기
            if(waitKey(10) >= 0) break;
        }
    }
    beaconConnect.join();
    beaconDisconnect.join();
    faceComparison.join();
    return 0;
}
