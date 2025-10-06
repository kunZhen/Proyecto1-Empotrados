from flask import Flask, render_template, jsonify, Response, request
import subprocess
import time
from gpio_wrapper import VehicleController
import base64
from datetime import datetime
import threading

app = Flask(__name__)

# Inicializa el controlador del vehículo
try:
    vehicle = VehicleController()
    print("Vehicle controller initialized successfully")
except Exception as e:
    print(f"Error initializing vehicle: {e}")
    vehicle = None

last_frame = None
last_frame_lock = threading.Lock()

def generate_frames():
    """Genera frames MJPEG usando ffmpeg desde /dev/video0"""
    global last_frame
    
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
            
            while True:
                start = buffer.find(b'\xff\xd8')
                if start == -1:
                    buffer = buffer[-2:]
                    break
                
                end = buffer.find(b'\xff\xd9', start)
                if end == -1:
                    break
                
                frame = buffer[start:end+2]
                buffer = buffer[end+2:]
                
                # Guarda el último frame
                with last_frame_lock:
                    last_frame = frame
                
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

@app.route('/api/capture', methods=['POST'])
def capture_image():
    """Captura el frame actual del stream"""
    global last_frame
    
    try:
        with last_frame_lock:
            if last_frame is None:
                return jsonify({'error': 'No frame available'}), 500
            frame_data = last_frame
        
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"capture_{timestamp}.jpg"
        filepath = f"/root/captures/{filename}"
        
        # Crea directorio si no existe
        subprocess.run(['mkdir', '-p', '/root/captures'], check=True)
        
        # Guarda el frame
        with open(filepath, 'wb') as f:
            f.write(frame_data)
        
        # Convierte a base64 para mostrar
        img_data = base64.b64encode(frame_data).decode('utf-8')
        
        return jsonify({
            'status': 'success',
            'filename': filename,
            'image': img_data
        })
            
    except Exception as e:
        print(f"Exception in capture: {str(e)}")
        return jsonify({'error': str(e)}), 500

@app.route('/api/captures')
def list_captures():
    """Lista todas las capturas guardadas"""
    try:
        result = subprocess.run(['ls', '-1', '/root/captures/'], 
                               capture_output=True, text=True)
        files = result.stdout.strip().split('\n') if result.stdout else []
        return jsonify({'captures': files})
    except:
        return jsonify({'captures': []})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=False, threaded=True)