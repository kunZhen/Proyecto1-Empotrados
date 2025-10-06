import ctypes
import os
import threading
import time

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

gpiolib.pinMode.argtypes = [ctypes.c_int, ctypes.c_int]
gpiolib.pinMode.restype = ctypes.c_int

gpiolib.digitalWrite.argtypes = [ctypes.c_int, ctypes.c_int]
gpiolib.digitalWrite.restype = ctypes.c_int

# Clase PWM por software
class SoftwarePWM:
    def __init__(self, gpio, frequency=1000):
        self.gpio = gpio
        self.frequency = frequency
        self.duty_cycle = 0
        self.running = False
        self.thread = None
        self.lock = threading.Lock()
        
        # Configura GPIO como salida
        gpiolib.pinMode(gpio, 1)  # OUTPUT
        gpiolib.digitalWrite(gpio, 0)
        
    def start(self, duty_cycle=0):
        with self.lock:
            self.duty_cycle = duty_cycle
            if not self.running:
                self.running = True
                self.thread = threading.Thread(target=self._pwm_thread, daemon=True)
                self.thread.start()
    
    def change_duty_cycle(self, duty_cycle):
        with self.lock:
            self.duty_cycle = max(0, min(100, duty_cycle))
        
    def stop(self):
        with self.lock:
            self.running = False
        if self.thread:
            self.thread.join(timeout=0.5)
        gpiolib.digitalWrite(self.gpio, 0)
    
    def _pwm_thread(self):
        period = 1.0 / self.frequency
        while True:
            with self.lock:
                if not self.running:
                    break
                current_duty = self.duty_cycle
            
            if current_duty > 0:
                on_time = period * (current_duty / 100.0)
                off_time = period - on_time
                
                if on_time > 0.0001:  # Evita sleeps muy cortos
                    gpiolib.digitalWrite(self.gpio, 1)
                    time.sleep(on_time)
                
                if off_time > 0.0001:
                    gpiolib.digitalWrite(self.gpio, 0)
                    time.sleep(off_time)
            else:
                gpiolib.digitalWrite(self.gpio, 0)
                time.sleep(period)

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
        
        # Inicializa motores (sin PWM hardware)
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
        
        # Crea PWM por software para control de velocidad
        self.pwm_left = SoftwarePWM(12, frequency=1000)
        self.pwm_right = SoftwarePWM(13, frequency=1000)
        self.pwm_left.start(0)
        self.pwm_right.start(0)
        
        print("PWM software initialized for motors")
    
    def forward(self, speed=80):
        gpiolib.vehicleForward(ctypes.byref(self.vehicle), 100)
        self.pwm_left.change_duty_cycle(speed)
        self.pwm_right.change_duty_cycle(speed)
        return 0
    
    def backward(self, speed=80):
        gpiolib.vehicleBackward(ctypes.byref(self.vehicle), 100)
        self.pwm_left.change_duty_cycle(speed)
        self.pwm_right.change_duty_cycle(speed)
        return 0
    
    def left(self, speed=80):
        gpiolib.vehicleLeft(ctypes.byref(self.vehicle), 100)
        self.pwm_left.change_duty_cycle(0)
        self.pwm_right.change_duty_cycle(speed)
        return 0
    
    def right(self, speed=80):
        gpiolib.vehicleRight(ctypes.byref(self.vehicle), 100)
        self.pwm_left.change_duty_cycle(speed)
        self.pwm_right.change_duty_cycle(0)
        return 0
    
    def stop(self):
        gpiolib.vehicleStop(ctypes.byref(self.vehicle))
        self.pwm_left.change_duty_cycle(0)
        self.pwm_right.change_duty_cycle(0)
        return 0