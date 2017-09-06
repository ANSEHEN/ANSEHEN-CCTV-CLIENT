import face_recognition
import zmq
import os
import struct
import threading

# global list
crop_encodings = {}
known_encodings = []

filenames = []
my_mutex = threading.Lock()
my_mutex1= threading.Lock()

count = 0
match = None

# [target thread]
# -------------------------------------------------------------------------------------
# recv filename  
def target_thread():
       
    global my_mutex
    global count
    global match
    
    # [ zmq ]  
    context = zmq.Context()
    # Socket to talk th server
    print("[TARGET] --- Connecting... ")
    socket = context.socket(zmq.REP) #받는 부분이 REP
    socket.connect("tcp://localhost:5550")
  
    while True:
        print('[TARGET_thread]')                
        try:
            print("[TARGET] wait")
            message = socket.recv_string()
            
            print("[TARGET] message : ",message)           
            filenames.append(message)
            msg = 'send again'
            socket.send_string(msg)
        except zmq.error.ZMQError:
            print("[TARGET recv] error")


        # save to image diction
        target_face_encoding = {}
        
        print("[TARGET] number : ",count," : encoding")
        known_target_image = face_recognition.load_image_file(filenames[count])
        
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

# ------------------------------------------------------------------------------------



# [crop thread]
# ------------------------------------------------------------------------------------
def crop_thread():
    global match
    global count

    context = zmq.Context()
    #Socket to talk server
    print("[CROP] --- Connecting... ")
    socket = context.socket(zmq.REP)
    socket.connect("tcp://localhost:5560")
    while True:
        while len(filenames) > 0:
            print("[CROP thread]")
            try:
                print("[CROP] wait")
                crop_filenames = socket.recv_string()
                
                print("[CROP] message : ",crop_filenames)
                crop_message  = crop_filenames.split('_')
                crop_filenames = None
                
            except zmq.error.ZMQError:
                print("[CROP] Error")
           
            # crop_message[0], crop_message [1]
            crop_try = int(crop_message[0])
            crop_number = int(crop_message[1])+1
            act = crop_number
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
                    crop_encodings[i] = (face_recognition.face_encodings(crop_image)[0])
                except IndexError:
                    print('[CROP] cannot encoding face : ', i)

    # [compare thread]
    # ------------------------------------------------------------------------------------
            if(len(known_encodings)!= 0) and (len(crop_encodings) !=0):
                print("[comparing]")
                
                for i in crop_encodings.keys():
                    my_mutex.acquire()
                    results = face_recognition.compare_faces(known_encodings, crop_encodings[i], 0.4)
                    print("[ ", i, "comparing ]")
                    my_mutex.release()
                    for w in range(len(results)):
                        if results[w] == True:
                            print("[comparing] target = {}".format(results[w]) , "user : " , w)
                            print("[comparing] match user : ", filenames[w])
                            
                            my_mutex.acquire()
                            match = filenames[w]
                            my_mutex.release()
                            my_mutex1.acquire()
                            del(known_encodings[w])
                            os.remove(filenames[w])
                            del(filenames[w])
                            count -= 1
                            my_mutex1.release()
             
                crop_encodings.clear()
                for j in range(len(crop_name)):
                    os.remove(crop_name[j])
                    
            if len(crop_encodings) == 0:
                msg = 'send_again'
                socket.send_string(msg)     
                print('[CROP:COMPARING] END')               

        
# ------------------------------------------------------------------------------------

# [match_send_message]
# ------------------------------------------------------------------------------------

def match_send_message():
    global match
    
    print("[SEND MATCH MESSAGE]")
    print("[SEND MATCH MESSAGE] --- Connecting...")
    context = zmq.Context()
    socket = context.socket(zmq.REP)
    socket.connect("tcp://localhost:5570")
    
    while True:
        if match != None:
            ready = socket.recv_string()
            print(ready)
            my_mutex.acquire()
            msg = match
            my_mutex.release()
            socket.send_string(msg)
            print("[SEND MATCH MESSAGE] complete send message")
            my_mutex.acquire()
            match = None
            my_mutex.release()

# ------------------------------------------------------------------------------------
    
print("[ANSEHEN START]")

match_th = threading.Thread(target = match_send_message)
match_th.start()

target_th= threading.Thread(target = target_thread)
target_th.start()

crop_th = threading.Thread(target = crop_thread)
crop_th.start()
