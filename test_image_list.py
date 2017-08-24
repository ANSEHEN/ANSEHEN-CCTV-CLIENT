import face_recognition

crop_encoding = {}
known_encodings = []
filenames = ['./pic/3.jpg','./pic/z24.jpg']
# [target thread]
# -------------------------------------------------------------------------------------
def target():
    print("[target thread]")
    # recv filename
    

    # save to image diction
    target_face_encoding = {}
    for i in range(len(filenames)):
        print("number",i," : encoding")
        filename = filenames[i]
        known_target_image = face_recognition.load_image_file(filename)
        target_face_encoding[i] = face_recognition.face_encodings(known_target_image)[0]

    #known_encodings = []
    for j in range(len(filenames)):
        known_encodings.append(target_face_encoding[j])


# ------------------------------------------------------------------------------------



# [crop thread]
# ------------------------------------------------------------------------------------
def crop():
    print("[crop thread]")
    crop_messages = ['./pic/5_133.jpg','./pic/0.jpg','./pic/z23.jpg','./pic/obama-480p.jpg']

    #crop_encoding = {}
    for j in range(len(crop_messages)):
        print("number",j," : encoding")
        crop_filename = crop_messages[j]
        crop_image = face_recognition.load_image_file(crop_filename)
        try:
            crop_encoding[j] = face_recognition.face_encodings(crop_image)[0]
        except IndexError:
            print('cannot encoding face : ', j)

    
        

# ------------------------------------------------------------------------------------


# [comparing face]
# ------------------------------------------------------------------------------------
def comparing_face():
    print("[comparing]")
    for k in crop_encoding.keys():
        results = face_recognition.compare_faces(known_encodings, crop_encoding[k], 0.4)
        print(k," comparing")
        """
        for w in range(len(results)):
            print(w,':',"Found the target {}".format(results[w]))
        """
        for w in range(len(results)): # len(results) == len(known_encodings)
            if results[w] == True:
                print(" target = {}".format(results[w]) , w)
                #del known_encodings[w]
                print(filenames[w])
                
                
# ------------------------------------------------------------------------------------
while True:
    target()
    crop()
    comparing_face()
    