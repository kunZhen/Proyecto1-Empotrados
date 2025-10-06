from flask import Flask, render_template, jsonify, Response, request
import subprocess
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

def generate_frames():
    """Genera frames MJPEG usando ffmpeg desde /dev/video0"""
    cmd = ['ffmpeg', 
           '-f', 'v4l2',
           '-input_format', 'mjpeg',
           '-video_size', '640x480',
           '-framerate', '30',
           '-i', '/dev/video0',
           '-f', 'mjpeg',
           '-q:v', '5',
           '-']
    
    process = subprocess.Popen(cmd, 
                               stdout=subprocess.PIPE,
                               stderr=subprocess.DEVNULL, 
                               bufsize=10**6)
    
    buffer = b''
    print("Camera streaming started with ffmpeg")
    
    try:
        while True:
            chunk = process.stdout.read(4096)
            if not chunk:
                break
            
            buffer += chunk
            
            # Busca frames JPEG completos
            while True:
                start = buffer.find(b'\xff\xd8')  # Inicio JPEG
                if start == -1:
                    buffer = buffer[-2:]
                    break
                
                end = buffer.find(b'\xff\xd9', start)  # Fin JPEG
                if end == -1:
                    break
                
                frame = buffer[start:end+2]
                buffer = buffer[end+2:]
                
                yield (b'--frame\r\n'
                       b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')
    except Exception as e:
        print(f"Stream error: {e}")
    finally:
        process.terminate()

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/video_feed')
def video_feed():
    print("Video feed requested")
    return Response(generate_frames(),
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
        'camera_active': True
    })

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=False, threaded=True)