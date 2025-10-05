import ctypes
import os

# Carga la biblioteca compartida
lib_path = "/root/gpioio/lib/libgpioio.so.1"
gpiolib = ctypes.CDLL(lib_path)

# Define estructuras C en Python
class VehicleLEDs(ctypes.Structure):
    _fields_ = [
        ("led_front_left", ctypes.c_int),
        ("led_front_right", ctypes.c_int),
        ("led_back_left", ctypes.c_int),
        ("led_back_right", ctypes.c_int),
        ("is_initialized", ctypes.c_int)
    ]

class Motor(ctypes.Structure):
    _fields_ = [
        ("pin_in1", ctypes.c_int),
        ("pin_in2", ctypes.c_int),
        ("pin_enable", ctypes.c_int),
        ("pwm_chip", ctypes.c_int),
        ("pwm_channel", ctypes.c_int),
        ("use_pwm", ctypes.c_int),
        ("is_initialized", ctypes.c_int),
        ("leds", ctypes.POINTER(VehicleLEDs))
    ]

class Vehicle(ctypes.Structure):
    _fields_ = [
        ("motor_left", ctypes.POINTER(Motor)),
        ("motor_right", ctypes.POINTER(Motor)),
        ("leds", ctypes.POINTER(VehicleLEDs))
    ]

# Define tipos de retorno y argumentos
gpiolib.ledsInit.argtypes = [
    ctypes.POINTER(VehicleLEDs),
    ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int
]
gpiolib.ledsInit.restype = ctypes.c_int

gpiolib.motorInit.argtypes = [
    ctypes.POINTER(Motor),
    ctypes.c_int, ctypes.c_int, ctypes.c_int,
    ctypes.POINTER(VehicleLEDs)
]
gpiolib.motorInit.restype = ctypes.c_int

gpiolib.vehicleInit.argtypes = [
    ctypes.POINTER(Vehicle),
    ctypes.POINTER(Motor),
    ctypes.POINTER(Motor),
    ctypes.POINTER(VehicleLEDs)
]
gpiolib.vehicleInit.restype = ctypes.c_int

gpiolib.vehicleForward.argtypes = [ctypes.POINTER(Vehicle), ctypes.c_int]
gpiolib.vehicleForward.restype = ctypes.c_int

gpiolib.vehicleBackward.argtypes = [ctypes.POINTER(Vehicle), ctypes.c_int]
gpiolib.vehicleBackward.restype = ctypes.c_int

gpiolib.vehicleLeft.argtypes = [ctypes.POINTER(Vehicle), ctypes.c_int]
gpiolib.vehicleLeft.restype = ctypes.c_int

gpiolib.vehicleRight.argtypes = [ctypes.POINTER(Vehicle), ctypes.c_int]
gpiolib.vehicleRight.restype = ctypes.c_int

gpiolib.vehicleStop.argtypes = [ctypes.POINTER(Vehicle)]
gpiolib.vehicleStop.restype = ctypes.c_int

# Wrapper class
class VehicleController:
    def __init__(self):
        # Pines según tu configuración
        self.leds = VehicleLEDs()
        self.motor_left = Motor()
        self.motor_right = Motor()
        self.vehicle = Vehicle()
        
        # Inicializa LEDs
        ret = gpiolib.ledsInit(
            ctypes.byref(self.leds),
            23,  # LED_FRONT_L
            24,  # LED_FRONT_R
            25,  # LED_BACK_L
            8    # LED_BACK_R
        )
        if ret < 0:
            raise Exception("Failed to initialize LEDs")
        
        # Inicializa motores
        ret = gpiolib.motorInit(
            ctypes.byref(self.motor_left),
            18, 19, 12,  # Motor izquierdo
            ctypes.byref(self.leds)
        )
        if ret < 0:
            raise Exception("Failed to initialize left motor")
        
        ret = gpiolib.motorInit(
            ctypes.byref(self.motor_right),
            20, 21, 13,  # Motor derecho
            ctypes.byref(self.leds)
        )
        if ret < 0:
            raise Exception("Failed to initialize right motor")
        
        # Inicializa vehículo
        gpiolib.vehicleInit(
            ctypes.byref(self.vehicle),
            ctypes.byref(self.motor_left),
            ctypes.byref(self.motor_right),
            ctypes.byref(self.leds)
        )
    
    def forward(self, speed=80):
        return gpiolib.vehicleForward(ctypes.byref(self.vehicle), speed)
    
    def backward(self, speed=80):
        return gpiolib.vehicleBackward(ctypes.byref(self.vehicle), speed)
    
    def left(self, speed=80):
        return gpiolib.vehicleLeft(ctypes.byref(self.vehicle), speed)
    
    def right(self, speed=80):
        return gpiolib.vehicleRight(ctypes.byref(self.vehicle), speed)
    
    def stop(self):
        return gpiolib.vehicleStop(ctypes.byref(self.vehicle))