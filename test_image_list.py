import face_recognition
import zmq
import os
import struct
import threading

# global list
crop_encoding = {}
known_encodings = []

filenames = []
my_mutex = threading.Lock()


# [target thread]
# -------------------------------------------------------------------------------------
# recv filename  
def zero_message():
       
    global my_mutex
    i = 0
    while True:
        
        # [ zmq ] 
        context = zmq.Context()
        # Socket to talk th server
        print("Connecting... ")
        socket = context.socket(zmq.REP) #받는 부분이 REP
        socket.connect("tcp://localhost:5555")
        try:
            print("wait")
            message = socket.recv_string()
            
            print("[recv]message : ",message)           
            filenames.append(message)
            
        except zmq.error.ZMQError:
            print("error")


        # save to image diction
        target_face_encoding = {}
        print("[target]number",i," : encoding")
        
        
        known_target_image = face_recognition.load_image_file(filenames[i])
        try:
            my_mutex.acquire()
            target_face_encoding[i] = face_recognition.face_encodings(known_target_image)[0]
            known_encodings.append(target_face_encoding[i])
            my_mutex.release()
            
        except IndexError:
            print('[target]cannot encoding face : ', i)
            # user Error
        i+=1
            
# ------------------------------------------------------------------------------------



# [crop thread]
# ------------------------------------------------------------------------------------
def crop():
    print("[crop thread]")
    a ='./pic/5_133.jpg'
    b='./pic/0.jpg'
    c='./pic/z23.jpg'
    
    crop_messages = [a,b,c]
    

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
        my_mutex.acquire()
        results = face_recognition.compare_faces(known_encodings, crop_encoding[k], 0.4)
        my_mutex.release()
        print(k," comparing")

        for w in range(len(results)): # len(results) == len(known_encodings)
            if results[w] == True:
                print(" [comparing]target = {}".format(results[w]) , w)
                #del known_encodings[w]
                print(filenames[w])
                
                
                #msg = filenames[w]
                #socket.send_string(msg)
                
                
# ------------------------------------------------------------------------------------
def run_process():
    while True:
    
        crop()
        #my_mutex.acquire()
        comparing_face()
        #my_mutex.release()
print("start")
#my_mutex.acquire()

t_run = threading.Thread(target = run_process)
t_run.start()
print('thread')
t_message = threading.Thread(target = zero_message)
t_message.run()




    