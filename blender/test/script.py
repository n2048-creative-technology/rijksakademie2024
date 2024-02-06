import bpy
import serial 
import threading
import time

arduino = serial.Serial(port='/dev/cu.usbserial-1420', baudrate=19200)

def send_data():
    
    cam = bpy.data.objects['Camera']
    scene = bpy.data.scenes['Scene']
    fps =  scene.render.fps;
    startFrame = scene.frame_start
    endFrame = scene.frame_end
    prevFrame = scene.frame_current 

    while True:
        current_frame = scene.frame_current        
        if(current_frame != prevFrame):
            prevFrame = current_frame
            # Get the current frame number
            pos_x = cam.location.x
            pos_y = cam.location.y
            pos_z = cam.location.z
            data = str(f"\tTime: %.2f\n%.1f,%.1f,%.1f" % (current_frame/fps, pos_x, pos_y, pos_z))
            
            arduino.write(data.encode())  # Add a newline character to indicate the end of the message

thread = threading.Thread(target=send_data)
thread.start()