## include learning 

import bpy

from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, PointerProperty, EnumProperty
from bpy.types import Operator
import struct
import math
import threading
import time
import serial 
import serial.tools.list_ports
import zlib

# Function to get the list of available serial ports
def get_serial_ports(self, context):
    items = []
    ports = serial.tools.list_ports.comports()
    for port in ports:
        items.append((port.device, port.device, port.description))
    return items

class ScaleFactors:
    def __init__(self, x1,x2,x3,x4,x5,x6,x7,x8,x9):
        self.x1 = x1
        self.x2 = x2
        self.x3 = x3
        self.x4 = x4
        self.x5 = x5
        self.x6 = x6
        self.x7 = x7
        self.x8 = x8
        self.x9 = x9
        
class SerialManager:
    def __init__(self):
        self.running = False
        self.learning = False
        self.thread = None
        self.ser = None
        self.selected_object = None
        self.scale_factors = None
        self.prev_data = None
        self.start_marker = b'<'
        self.end_marker = b'>'
        self.lock = threading.Lock()  # Create a lock


    def connect(self, serial_port):
        print(f"connecting...")
        if self.ser is not None and not self.ser.is_open:  
            self.ser = serial.Serial(serial_port, 115200, timeout=1)
            time.sleep(2)

    def disconnect(self, serial_port):
        self.stop()
        print(f"disconnecting...")
        if self.ser is not None and not self.ser.is_open: 
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
            self.ser.close()

    def learn(self, serial_port, selected_object, scale_factors):
        print(f"learning...")
        if not self.running and not self.learning:

            self.learning = True
            self.running = False

            self.ser = serial.Serial(serial_port, 115200, timeout=1)
            self.thread = threading.Thread(target=self.run)
            self.thread.start()
        
                
    def start(self, serial_port, selected_object, scale_factors):
        if not self.running and not self.learning:

            self.learning = False
            self.running = True
            self.prev_checksum = None; # so it sends the current position
            self.selected_object = selected_object
            self.scale_factors = scale_factors
            self.ser = serial.Serial(serial_port, 115200, timeout=1)

            time.sleep(2)
            
            self.thread = threading.Thread(target=self.run)
            self.thread.start()

    def stop(self):
        if self.running or self.learning:
            self.running = False
            self.learning = False
            
            with self.lock:  # Use lock to ensure thread-safe access
#                if self.ser is not None and self.ser.is_open:                    
#                    self.ser.reset_input_buffer()
#                    self.ser.reset_output_buffer()
#                    self.ser.close()
                self.prev_checksum = None  # Clear the previous checksum when stopping

            if self.thread is not None:
                self.thread.join(timeout=5)  # Wait a maximum of 5 seconds for the thread to terminate


    def sendData(self):

        with self.lock:  # Use lock to ensure thread-safe access
            if not self.running or self.ser is None:
                return

            loc = self.selected_object.location
            rot = self.selected_object.rotation_euler
            scale = self.selected_object.scale

            data = struct.pack('9f', 
                loc.x * self.scale_factors.x1,
                loc.y * self.scale_factors.x2,
                loc.z * self.scale_factors.x3,
                math.degrees(rot.x) * self.scale_factors.x4,
                math.degrees(rot.y) * self.scale_factors.x5,
                math.degrees(rot.z) * self.scale_factors.x6,
                scale.x * self.scale_factors.x7,
                scale.y * self.scale_factors.x8,
                scale.z * self.scale_factors.x9                  
            )   
            checksum = zlib.crc32(data)  & 0xFFFFFFFF
            if checksum != self.prev_checksum: 
                message = self.start_marker + data + struct.pack('I', checksum) + self.end_marker
                self.ser.write(message)
                self.prev_checksum = checksum           

    def receiveData(self):
        with self.lock:  # Use lock to ensure thread-safe access
            if not self.learning or self.ser is None:
                return
            data = b'?'
            message = self.start_marker + data + self.end_marker
            self.ser.write(message)
            time.sleep(.5)  

            # Read a line from the serial port
            data = self.ser.readline().decode('utf-8').strip()
            if data:  # Check if data is not empty
                # Split the data by commas and convert to float
                positions = list(map(float, data.split(',')))
                print(positions)  # Display the received float array
                
                self.selected_object.location.x = positions[0] / self.scale_factors.x1
                self.selected_object.location.y = positions[1] / self.scale_factors.x2
                self.selected_object.location.z = positions[2] / self.scale_factors.x3
                
                self.selected_object.rotation_euler.x  = math.radians(positions[3]) / self.scale_factors.x4
                self.selected_object.rotation_euler.y  = math.radians(positions[4]) / self.scale_factors.x5
                self.selected_object.rotation_euler.z  = math.radians(positions[5]) / self.scale_factors.x6

                self.selected_object.scale.x = positions[6] / self.scale_factors.x7
                self.selected_object.scale.y = positions[7] / self.scale_factors.x8
                self.selected_object.scale.z = positions[8] / self.scale_factors.x9   
                   
                with bpy.context.temp_override(object=self.selected_object):
                    self.selected_object.keyframe_insert(data_path="location")
                    self.selected_object.keyframe_insert(data_path="rotation_euler")
                    self.selected_object.keyframe_insert(data_path="scale")
          
                            
    def run(self):  
        retries = 0
        max_retries = 5  # Set a retry limit
        try:
            with self.lock:
                self.ser.reset_input_buffer()
                self.ser.reset_output_buffer()

            while ( self.running or self.learning) and retries < max_retries:   
                if not self.ser.is_open:  # Check if serial port is still open
                    print("Serial port disconnected. Attempting to reconnect.")
                    retries += 1
                    self.stop()
                    time.sleep(1)  # Brief pause before restarting
                    if retries < max_retries:
                        self.start(self.ser.port, self.selected_object, self.scale_factors)
                        continue
                    else:
                        print("Max retries reached, stopping.")
                        self.running = False
                        self.learning = False
                        return  # Exit the loop if max retries are reached


                if self.selected_object:
                    try:
                        if self.running: 
                            self.sendData()
                        if self.learning:
                            self.receiveData()                            
                        retries = 0  # Reset retries if successful
                    except (serial.SerialTimeoutException, serial.SerialException) as e:
                        print(f"Serial error: {e}")
                        retries += 1
                        self.stop()
                        time.sleep(1)  # Brief pause before restarting
                        if retries < max_retries:
                            self.start(self.ser.port, self.selected_object, self.scale_factors)
                            continue
                        else:
                            print("Max retries reached, stopping.")
                            self.running = False
                            self.learning = False
                        self.start(self.ser.port, self.selected_object, self.scale_factors)
                        continue
                    fps = bpy.context.scene.render.fps
                    if fps > 0:
                        time.sleep(1.0 / fps)
                    else:
                        time.sleep(0.1)  # Use a default small delay if FPS is invalid                    

        except Exception as e:
            print(f"Error in thread: {e}")
            self.stop()


class LearnButtonOperator(bpy.types.Operator):
    bl_idname = "object.learn_button"
    bl_label = "Learn Button"

    def execute(self, context):                    
        
        # Connect to selected serial Port

        serial_manager = context.scene.serial_manager
        selected_object = context.scene.selected_object
        serial_port = context.scene.serial_port
        
        scale_factors = ScaleFactors(
            context.scene.scale_factor_1,
            context.scene.scale_factor_2,
            context.scene.scale_factor_3,
            context.scene.scale_factor_4,
            context.scene.scale_factor_5,
            context.scene.scale_factor_6,
            context.scene.scale_factor_7,
            context.scene.scale_factor_8,
            context.scene.scale_factor_9,
        )
        
        
        serial_manager.learn(serial_port, selected_object, scale_factors);
        
        context.scene.running = False    
        context.scene.learning = True    
        
        self.report({'INFO'}, "Learn")
        
        time.sleep(1)
        serial_manager.stop()
        self.report({'INFO'}, "Stopped")
       
        context.scene.learning = False   
        
        return {'FINISHED'}

class ConnectButtonOperator(bpy.types.Operator):
    bl_idname = "object.connect_button"
    bl_label = "Connect Button"

    def execute(self, context):                    
        
        # Connect to selected serial Port
        serial_manager = context.scene.serial_manager
        serial_port = context.scene.serial_port

        serial_manager.connect(serial_port)
        
        # Toggle the button state
        context.scene.connected = True         
            
        self.report({'INFO'}, "Connecting")
            
        return {'FINISHED'}


class DisconnectButtonOperator(bpy.types.Operator):
    bl_idname = "object.disconnect_button"
    bl_label = "Disonnect Button"

    def execute(self, context):                    
        
        # Connect to selected serial Port
        serial_manager = context.scene.serial_manager
        serial_port = context.scene.serial_port

        serial_manager.disconnect(serial_port)

        # Toggle the button state
        context.scene.connected = False    
        context.scene.running = False
              
        if bpy.context.screen.is_animation_playing:
            bpy.ops.screen.animation_play()
            
        self.report({'INFO'}, "Disconnecting")
            
        return {'FINISHED'}
    
class RunButtonOperator(bpy.types.Operator):
    bl_idname = "object.run_button"
    bl_label = "Run Button"

    def execute(self, context):                    
        
        if not context.scene.connected: 
            return {'FINISHED'}
        
        # Connect to selected serial Port
        serial_manager = context.scene.serial_manager
        serial_port = context.scene.serial_port
        selected_object = context.scene.selected_object
        
        scale_factors = ScaleFactors(
            context.scene.scale_factor_1,
            context.scene.scale_factor_2,
            context.scene.scale_factor_3,
            context.scene.scale_factor_4,
            context.scene.scale_factor_5,
            context.scene.scale_factor_6,
            context.scene.scale_factor_7,
            context.scene.scale_factor_8,
            context.scene.scale_factor_9,
        )
        
        if selected_object:
            serial_manager.start(serial_port, selected_object, scale_factors)
            self.report({'INFO'}, "Running")
        else:
            self.report({'WARNING'}, "No object selected")

        context.scene.running = True    
        context.scene.learning = False    
            
        # Toggle the button state
        #if not bpy.context.screen.is_animation_playing:
        #    bpy.ops.screen.animation_play()
                        
        self.report({'INFO'}, "Running")
            
        return {'FINISHED'}


class StopButtonOperator(bpy.types.Operator):
    bl_idname = "object.stop_button"
    bl_label = "Stop Button"

    def execute(self, context):

        # Toggle the button state
        context.scene.running = False
        context.scene.learning = False
        
        if bpy.context.screen.is_animation_playing:
            bpy.ops.screen.animation_play()
        
        serial_manager = context.scene.serial_manager
        if serial_manager.running:
            serial_manager.stop()
            self.report({'INFO'}, "Stopped")
                            
        return {'FINISHED'}



class LayoutRABKPanel(bpy.types.Panel):
    bl_label = "RABK Automation Runner"
    bl_idname = "SCENE_PT_layout"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "scene"

    def draw(self, context):
        
        layout = self.layout

        scene = context.scene

        # Create a simple row. 
        layout.label(text="Automation frames:")

        row = layout.row()
        row.prop(scene, "frame_start")
        row.prop(scene, "frame_end")        
                
        row = layout.row()
        row.prop_search(context.scene, "selected_object", context.scene, "objects")

        layout.label(text="Scale:")
        layout.prop(scene, "scale_factor_1", text="Scale factor 1")
        layout.prop(scene, "scale_factor_2", text="Scale factor 2")
        layout.prop(scene, "scale_factor_3", text="Scale factor 3")
        layout.prop(scene, "scale_factor_4", text="Scale factor 4")
        layout.prop(scene, "scale_factor_5", text="Scale factor 5")
        layout.prop(scene, "scale_factor_6", text="Scale factor 6")
        layout.prop(scene, "scale_factor_7", text="Scale factor 7")
        layout.prop(scene, "scale_factor_8", text="Scale factor 8")
        layout.prop(scene, "scale_factor_9", text="Scale factor 9")
        

        layout.label(text="Connection:")
        
        row = layout.row()
        # Dropdown menu to select serial port
        row.prop(context.scene, "serial_port", text="Port")

        if context.scene.connected == False:
            row.operator("object.connect_button", text="Connect")
        else:
            row.operator("object.disconnect_button", text="Disconnect")

        # Run animation on serial port
        layout.label(text="Run automation:")

        row = layout.row()        
        row.scale_y = 3.0
        if context.scene.running == False:
            row.operator("object.run_button", text="Run")
        else:
            row.operator("object.stop_button", text="Stop")

        # Learn positions from arduino
         
        layout.label(text="Learn:")

        row = layout.row()        
        row.scale_y = 3.0
        if context.scene.learning == False:
            row.operator("object.learn_button", text="Learn")    
        else:
            row.operator("object.stop_button", text="Stop")


def register(): 
    bpy.utils.register_class(RunButtonOperator)
    bpy.utils.register_class(StopButtonOperator)
    bpy.utils.register_class(LearnButtonOperator)
    bpy.utils.register_class(ConnectButtonOperator)
    bpy.utils.register_class(DisconnectButtonOperator)
    
    bpy.types.Scene.selected_object = PointerProperty(
        type=bpy.types.Object, 
        name="Automated Object"
    )

    bpy.types.Scene.scale_factor_1 = bpy.props.FloatProperty(
        name="scale factor 1",
        description="the ratio between the change in blender and the setpoint change of the motor",
        default=1.0,
        min=0.0
    )
    bpy.types.Scene.scale_factor_2 = bpy.props.FloatProperty(
        name="scale factor 2",
        description="the ratio between the change in blender and the setpoint change of the motor",
        default=1.0,
        min=0.0
    )
    bpy.types.Scene.scale_factor_3 = bpy.props.FloatProperty(
        name="scale factor 3",
        description="the ratio between the change in blender and the setpoint change of the motor",
        default=1.0,
        min=0.0
    )
    bpy.types.Scene.scale_factor_4 = bpy.props.FloatProperty(
        name="scale factor 4",
        description="the ratio between the change in blender and the setpoint change of the motor",
        default=1.0,
        min=0.0
    )
    bpy.types.Scene.scale_factor_5 = bpy.props.FloatProperty(
        name="scale factor 5",
        description="the ratio between the change in blender and the setpoint change of the motor",
        default=1.0,
        min=0.0
    )
    bpy.types.Scene.scale_factor_6 = bpy.props.FloatProperty(
        name="scale factor 6",
        description="the ratio between the change in blender and the setpoint change of the motor",
        default=1.0,
#        max=10.0,
        min=0.0
    )
    bpy.types.Scene.scale_factor_7 = bpy.props.FloatProperty(
        name="scale factor 7",
        description="the ratio between the change in blender and the setpoint change of the motor",
        default=1.0,
#        max=10.0,
        min=0.0
    )    
    bpy.types.Scene.scale_factor_8 = bpy.props.FloatProperty(
        name="scale factor 8",
        description="the ratio between the change in blender and the setpoint change of the motor",
        default=1.0,
#        max=10.0,
        min=0.0
    )
    bpy.types.Scene.scale_factor_9 = bpy.props.FloatProperty(
        name="scale factor 9",
        description="the ratio between the change in blender and the setpoint change of the motor",
        default=1.0,
#        max=10.0,
        min=0.0
    )
    
    bpy.types.Scene.running = bpy.props.BoolProperty(
        default=False
    )    
    bpy.types.Scene.learning = bpy.props.BoolProperty(
        default=False
    )    
    bpy.types.Scene.connected = bpy.props.BoolProperty(
        default=False
    )
    
    # Add properties to the scene
    bpy.types.Scene.serial_port = bpy.props.EnumProperty(items=get_serial_ports, name="Port")

    bpy.types.Scene.serial_manager = SerialManager()

    bpy.utils.register_class(LayoutRABKPanel)
    
    
def unregister():    
    bpy.utils.unregister_class(LayoutRABKPanel)  
    bpy.utils.unregister_class(RunButtonOperator)
    bpy.utils.unregister_class(StopButtonOperator)
    bpy.utils.unregister_class(LearnButtonOperator)
    bpy.utils.unregister_class(ConnectButtonOperator)
    bpy.utils.unregister_class(DisconnectButtonOperator)

    # Remove properties from the scene
    del bpy.types.Scene.selected_object
    
    del bpy.types.Scene.scale_factor_1
    del bpy.types.Scene.scale_factor_2
    del bpy.types.Scene.scale_factor_3
    del bpy.types.Scene.scale_factor_4
    del bpy.types.Scene.scale_factor_5
    del bpy.types.Scene.scale_factor_6
    del bpy.types.Scene.scale_factor_7
    del bpy.types.Scene.scale_factor_8
    del bpy.types.Scene.scale_factor_9
    
    del bpy.types.Scene.running   
    del bpy.types.Scene.learning   

    del bpy.types.Scene.connected   
         
    del bpy.types.Scene.serial_port
    del bpy.types.Scene.serial_manager

if __name__ == "__main__":
    register()   
    
