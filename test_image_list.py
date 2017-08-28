import face_recognition
import zmq
import os
import struct
import threading




# global list
crop_encoding = {}
known_encodings = []


message = []
temp = None
filenames = []
my_mutex = threading.Lock()
# [target thread]
# -------------------------------------------------------------------------------------
    # recv filename  
def zero_message():
       
    global temp
    global my_mutex
    while True:
        
        # [ zmq ] 
        context = zmq.Context()
        # Socket to talk th server
        print("Connecting... ")
        socket = context.socket(zmq.REP) #받는 부분이 REP
        socket.connect("tcp://localhost:5555")
        try:
            print("wait")
            message1 = socket.recv_string()
            
            temp = message1
            print("[recv]message : ",temp)
            my_mutex.release()
            
        except zmq.error.ZMQError:
            print("error")
             
def target():
    print("[target thread]")
    global message
    global temp
    global my_mutex
    
    my_mutex.acquire()
    if temp == None:
        return 0
    elif temp != message :
        message = temp
        print("target : temp : ", temp)
        filenames.append(message)
    
    else:
        return 0
    
    # save to image diction
    target_face_encoding = {}
    for i in range(len(filenames)):
        print("[target]number",i," : encoding")
        filename = filenames[i]
        print("[target]filename",i," : ",filename)
        known_target_image = face_recognition.load_image_file(filename)
        try:
            target_face_encoding[i] = face_recognition.face_encodings(known_target_image)[0]
        except IndexError:
            print('[target]cannot encoding face : ', i)
        

    #known_encodings = []
    for j in range(len(filenames)):
        known_encodings.append(target_face_encoding[j])


# ------------------------------------------------------------------------------------



# [crop thread]
# ------------------------------------------------------------------------------------
def crop():
    print("[crop thread]")
    crop_messages = ['./pic/5_133.jpg','./pic/0.jpg','./pic/z23.jpg']

    #crop_encoding = {}
    for j in range(len(crop_messages)):
        print("[crop]number",j," : encoding")
        crop_filename = crop_messages[j]
        crop_image = face_recognition.load_image_file(crop_filename)
        try:
            crop_encoding[j] = face_recognition.face_encodings(crop_image)[0]
        except IndexError:
            print('[crop]cannot encoding face : ', j)

    
        

# ------------------------------------------------------------------------------------


# [comparing face]
# ------------------------------------------------------------------------------------
def comparing_face():
    print("[comparing]")
    for k in crop_encoding.keys():
        results = face_recognition.compare_faces(known_encodings, crop_encoding[k], 0.4)
        print(k," comparing")

        for w in range(len(results)): # len(results) == len(known_encodings)
            if results[w] == True:
                print(" [compareing]target = {}".format(results[w]) , w)
                #del known_encodings[w]
                print(filenames[w])
                #msg = filenames[w]
                #socket.send_string(msg)
                
                
# ------------------------------------------------------------------------------------
def run_process():
    while True:
        #zero_message()
        target()
        crop()
        comparing_face()
print("start")
#my_mutex.acquire()
my_mutex.acquire()
tr = threading.Thread(target = run_process)
tr.start()
print('thread')
t = threading.Thread(target = zero_message)
#t.daemon = True
t.run()




    