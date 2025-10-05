from flask import Flask, render_template, jsonify, Response, request
import subprocess
import threading
import time
from gpio_wrapper import VehicleController

app = Flask(__name__)

# Inicializa el controlador del vehículo
try:
    vehicle = VehicleController()
    print("Vehicle controller initialized successfully")
except Exception as e:
    print(f"Error initializing vehicle: {e}")
    vehicle = None

class Camera:
    def __init__(self):
        self.process = None
        self.start()
    
    def start(self):
        if self.process is None:
            self.process = subprocess.Popen([
                'libcamera-vid', '-t', '0',
                '--codec', 'mjpeg',
                '--width', '640', '--height', '480',
                '-o', '-', '--nopreview'
            ], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, bufsize=0)
    
    def get_frame(self):
        # Lee hasta encontrar inicio de JPEG (0xFF 0xD8)
        while True:
            byte = self.process.stdout.read(1)
            if not byte:
                return None
            if byte == b'\xff':
                next_byte = self.process.stdout.read(1)
                if next_byte == b'\xd8':  # Inicio JPEG
                    # Lee hasta fin de JPEG (0xFF 0xD9)
                    frame = b'\xff\xd8'
                    while True:
                        byte = self.process.stdout.read(1)
                        if not byte:
                            return None
                        frame += byte
                        if byte == b'\xff':
                            next_byte = self.process.stdout.read(1)
                            if not next_byte:
                                return None
                            frame += next_byte
                            if next_byte == b'\xd9':  # Fin JPEG
                                return frame

camera = Camera()

def gen_frames():
    while True:
        frame = camera.get_frame()
        if frame:
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/video_feed')
def video_feed():
    return Response(gen_frames(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/api/control', methods=['POST'])
def control():
    if not vehicle:
        return jsonify({'error': 'Vehicle not initialized'}), 500
    
    data = request.json
    action = data.get('action')
    speed = data.get('speed', 80)
    
    result = 0
    if action == 'forward':
        result = vehicle.forward(speed)
    elif action == 'backward':
        result = vehicle.backward(speed)
    elif action == 'left':
        result = vehicle.left(speed)
    elif action == 'right':
        result = vehicle.right(speed)
    elif action == 'stop':
        result = vehicle.stop()
    else:
        return jsonify({'error': 'Invalid action'}), 400
    
    if result < 0:
        return jsonify({'error': 'Command failed'}), 500
    
    return jsonify({'status': 'success', 'action': action, 'speed': speed})

@app.route('/api/status')
def status():
    return jsonify({
        'vehicle_initialized': vehicle is not None,
        'camera_active': camera.process is not None
    })

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=False, threaded=True)