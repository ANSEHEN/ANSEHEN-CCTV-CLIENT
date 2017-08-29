import face_recognition
import zmq
import os
import struct
import threading

# global list
crop_encodings = []
known_encodings = []
i = 0
filenames = []
my_mutex = threading.Lock()
match = None

# [target thread]
# -------------------------------------------------------------------------------------
# recv filename  
def target_():
       
    global my_mutex
    global i
    while True:
        print('[target thread]')
        # [ zmq ] 
        context = zmq.Context()
        # Socket to talk th server
        print("target---Connecting... ")
        socket = context.socket(zmq.REP) #받는 부분이 REP
        socket.connect("tcp://localhost:5550")
        try:
            print("target_wait")
            message = socket.recv_string()
            
            print("[target]message : ",message)           
            filenames.append(message)
            
        except zmq.error.ZMQError:
            print("error")


        # save to image diction
        target_face_encoding = {}
        print("[target]number",i," : encoding")
        
        my_mutex.acquire()
        known_target_image = face_recognition.load_image_file(filenames[i])
        my_mutex.release()
        try:
            my_mutex.acquire()
            target_face_encoding = face_recognition.face_encodings(known_target_image)[0]
            known_encodings.append(target_face_encoding)
            my_mutex.release()
            
        except IndexError:
            print('[target]cannot encoding face : ', i)
            # user Error
        my_mutex.acquire()
        i+=1
        my_mutex.release()
        
    print('target_end')
# ------------------------------------------------------------------------------------



# [crop thread]
# ------------------------------------------------------------------------------------
def crop():
    while True:
        print("[crop thread]")  
        crop_messages = []
        filename=[]
        context = zmq.Context()
        # Socket to talk th server
        print("crop---Connecting... ")
        socket = context.socket(zmq.REP) #받는 부분이 REP
        socket.connect("tcp://localhost:5560")
        try:
            print("crop_wait")
            crop_message = socket.recv_string()
                
            print("[crop]message : ",crop_message)           
            filename.append(crop_message)
                
        except zmq.error.ZMQError:
            print("error")
        
        

        #crop_encoding = {}
        for j in range(len(filename)):
            print("range: ",len(filename))
            print("[crop]number",j," : encoding")
            crop_image = face_recognition.load_image_file(filename[j])
            try:
                my_mutex.acquire()
                crop_encodings.append(face_recognition.face_encodings(crop_image)[0])
                my_mutex.release()
            except IndexError:
                print('[crop]cannot encoding face : ', j)
        print('crop_end')
        
        

# ------------------------------------------------------------------------------------

"""
# [comparing face]
# ------------------------------------------------------------------------------------
def comparing_face():
    print("[comparing]")
    print("comparing_face---Connecting... ") 
    #print('crop_messages:',crop_message)
    while known_encodings != None and crop_encoding != None:
        context = zmq.Context()
        # Socket to talk th server
        socket = context.socket(zmq.REQ) #받는 부분이 REP
        socket.connect("tcp://localhost:5570")
        for k in crop_encoding.keys():
            my_mutex.acquire()
            results = face_recognition.compare_faces(known_encodings, crop_encoding[k], 0.4)
            my_mutex.release()
            print(k," comparing")

            for w in range(len(results)): # len(results) == len(known_encodings)
                if results[w] == True:
                    print(" [comparing]target = {}".format(results[w]) , w)
                    #del known_encodings[w]
                    print(filenames[w])
                    #comparing_face_messages = socket.send_string(filenames[w])
                    print('comparing_face_message send')
                    
        #break
                    msg = filenames[w]
                    socket.send_string(msg)
                
                
# ------------------------------------------------------------------------------------
"""
# ------------------------------------------------------------------------------------
def comparing_face():
    global match
    global i
    print("[comparing]")
    print("comparing_face---Connecting... ") 
    #print('crop_messages:',crop_message)
    context = zmq.Context()
    # Socket to talk th server
    socket = context.socket(zmq.REQ) #받는 부분이 REP
    socket.connect("tcp://localhost:5570")
    if match !=None:
        my_mutex.acquire()
        msg = match
        socket.send_string(msg)
        my_mutex.release()
        match = None
        print("match")
        
def match_test():
    global match
    global i
    while True:
        while (len(known_encodings) != 0) and (len(crop_encodings) != 0):
            my_mutex.acquire()
            #if len(known_encodings) ==0:
                #break
            for k in range(len(crop_encodings)):
                results = face_recognition.compare_faces(known_encodings, crop_encodings[k], 0.4)
                my_mutex.release()
                print(k," comparing")
                for w in range(len(results)): # len(results) == len(known_encodings)
                    print('forforfor')
                    if results[w] == True:
                        print(" [comparing]target = {}".format(results[w]) , w)
                        #del known_encodings[w]
                        print(filenames[w])
                        my_mutex.acquire()
                        match = filenames[w]
                        del(known_encodings[w])
                        del(filenames[w])
                        i-=1
                        my_mutex.release()
                        #comparing_face_messages = socket.send_string(filenames[w])
                        print('comparing_face_message send')
                        comparing_face()
                    for x in range(len(crop_encodings)):
                        del(crop_encodings[x])

       
                        
# ------------------------------------------------------------------------------------
"""def run_process():
    while True:
        target_()
        crop()
        #my_mutex.acquire()
        #comparing_face()
        #my_mutex.release()
print("start")"""
#my_mutex.acquire()

t_target_ = threading.Thread(target = target_)
t_target_.start()
t_crop = threading.Thread(target = crop)
t_crop.start()
#t_comparing_face = threading.Thread(target = comparing_face)
#t_comparing_face.start()
t_match_test = threading.Thread(target = match_test)
t_match_test.start()
