import face_recognition
import zmq
import os
import struct
import threading

# global list
crop_encodings = []
known_encodings = []

filenames = []
my_mutex = threading.Lock()

count = 0
match = None
act = 0
# [target thread]
# -------------------------------------------------------------------------------------
# recv filename  
def target_thread():
       
    global my_mutex
    global count
    global match
    
    while True:
        print('[TARGET_thread]')
        # [ zmq ]
        
        context = zmq.Context()
        # Socket to talk th server
        print("[TARGET] --- Connecting... ")
        socket = context.socket(zmq.REP) #받는 부분이 REP
        socket.connect("tcp://localhost:5550")
        
        if match == None:         
            try:
                print("[TARGET] wait")
                message = socket.recv_string()
                match = 0
                
                print("[TARGET] message : ",message)           
                filenames.append(message)
                
            except zmq.error.ZMQError:
                print("[TARGET recv] error")


            # save to image diction
            target_face_encoding = {}
            my_mutex.acquire()
            print("[TARGET] number : ",count," : encoding")
            known_target_image = face_recognition.load_image_file(filenames[count])
            my_mutex.release()
            try:
                my_mutex.acquire()
                target_face_encoding= face_recognition.face_encodings(known_target_image)[0]
                known_encodings.append(target_face_encoding)
                my_mutex.release()
                
            except IndexError:
                print('[TARGET] cannot encoding face : ', count)
                # user Error
            count+=1
            print('[TARGET] END')
            
        else:           
            msg = match
            socket.send_string(msg)
            print("[SEND MATCH MESSAGE] complete send message")
            match = None
            
            
# ------------------------------------------------------------------------------------



# [crop thread]
# ------------------------------------------------------------------------------------
def crop_thread():
    global act
    while True:
        print("[CROP thread]")
        
        context = zmq.Context()
        #Socket to talk server
        print("[CROP] --- Connecting... ")
        socket = context.socket(zmq.REP)
        socket.connect("tcp://localhost:5560")
        
        try:
            print("[CROP] wait")
            crop_filenames = socket.recv_string()
            
            print("[CROP] message : ",crop_filenames)
            crop_message  = crop_filenames.split('_')
            
        except zmq.error.ZMQError:
            print("[CROP] Error")
       
        # crop_message[0], crop_message [1]
        crop_try = int(crop_message[0])
        crop_number = int(crop_message[1])+1
        crop_name = []
        string1 = "_"
        string2 = ".jpg"

        for i in range(crop_number):
            string_i = "{}".format(i)
            crop_name.append(crop_message[0] + string1 + string_i + string2)
            print("[CROP] filename : ", crop_name[i])       
            #crop_encoding = {}
            print("[CROP] try : ", crop_try ," , encoding : ", i)
            crop_image = face_recognition.load_image_file(crop_name[i])
            try:
                my_mutex.acquire()
                crop_encodings.append(face_recognition.face_encodings(crop_image)[0])
                my_mutex.release()
            except IndexError:
                print('[CROP] cannot encoding face : ', j)
        print('[CROP] END')

    
        

# ------------------------------------------------------------------------------------


# [comparing face]
# ------------------------------------------------------------------------------------
def comparing_thread():
    global match
    global count
    while True:
        if(len(known_encodings)!= 0) and (len(crop_encodings)!=0):
            print("[comparing]")
            
            for i in range(len(crop_encodings)):
                my_mutex.acquire()
                results = face_recognition.compare_faces(known_encodings, crop_encodings[i], 0.4)
                my_mutex.release()
                for w in range(len(results)):
                    if results[w] == True:
                        print("[comparing] target = {}".format(results[w]) , "user : " , w)
                        print("[comparing] match user : ", filenames[w])
                        
                        my_mutex.acquire()
                        match = filenames[w]
                        del(known_encodings[w])
                        del(filenames[w])
                        count -= 1
                        my_mutex.release()
                        #match_send_message()
                for x in range(len(crop_encodings)):
                    del(crop_encodings[0])
                    
            my_mutex.acquire()
            match = None
            my_mutex.release()
        
# ------------------------------------------------------------------------------------

# [match_send_message]
# ------------------------------------------------------------------------------------
"""
def match_send_message():
    global match
    
    print("[SEND MATCH MESSAGE]")
    print("[SEND MATCH MESSAGE] --- Connecting...")
    context = zmq.Context()
    socket = context.socket(zmq.REQ)
    socket.connect("tcp://localhost:5550")
    #my_mutex.acquire()
    msg = match
    #my_mutex.release()
    socket.send_string(msg)
    print("[SEND MATCH MESSAGE] complete send message")
"""

# ------------------------------------------------------------------------------------

print("[ANSEHEN START]")
#my_mutex.acquire()

target_th= threading.Thread(target = target_thread)
target_th.start()

crop_th = threading.Thread(target = crop_thread)
crop_th.start()

compare_th = threading.Thread(target = comparing_thread)
compare_th.start()

"""
------------------------------------------------------------------------------------------------------
[ client ]

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


int main(void)
{
	printf("[target]\n");
	
	char t_string[2][10] = {"obama.jpg","leeho.jpg"};
	
	
	for(int count = 0 ; count <2 ; count ++)
	{
		Data data;
		
		strcpy(data.image_addr, t_string[count]);
		
		void *context = zmq_ctx_new();
		void *responder = zmq_socket (context, ZMQ_REQ);
		int rc = zmq_bind(responder, "tcp://*:5550");

		char buffer [40] = {0,}, sbuff[40] = {0,} ,rbuff[40]={0,};

		strcpy(rbuff, data.image_addr);
		printf("rbuff: %s\n", rbuff);
		usleep(100);

		snprintf(sbuff, sizeof(sbuff), "%s", rbuff);
		printf("filename send : [ %s ]\n", sbuff);
		zmq_send (responder, sbuff, strlen(sbuff), 0);
		zmq_recv (responder, buffer, 20, 0);
		printf("매치 결과 : %s\n", buffer);

		
	}
	return 0;
}
------------------------------------------------------------------------------------------------------
[crop]


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

int main(void)
{
	printf("[crop]\n");
	char t_string[2][10] = {"0_4","1_2"};

	for(int i=0 ; i<2;i++)
	{
		Data data;
		strcpy(data.image_addr, t_string[i]);
		void *context = zmq_ctx_new();
		void *responder = zmq_socket (context, ZMQ_REQ);
		int rc = zmq_bind(responder, "tcp://*:5560");

		char buffer [40] = {0,}, sbuff[40] = {0,} ,rbuff[40]={0,};
		
		strcpy(rbuff, data.image_addr);
		printf("rbuff: %s\n", rbuff);
		usleep(100);
		snprintf(sbuff, sizeof(sbuff), "%s", rbuff);
		printf("filename send : [ %s ]\n", sbuff);
		zmq_send (responder, sbuff, strlen(sbuff), 0);
		usleep(20000);
	}
	return 0;
}
------------------------------------------------------------------------------------------------------
"""
