from flask import Blueprint, render_template, request, redirect, url_for, session, jsonify, Response
from werkzeug.security import generate_password_hash, check_password_hash
import json, os, subprocess
from .control import VehicleController

main = Blueprint("main", __name__)

USERS_FILE = "users.json"

# ------------------ USUARIOS ------------------
def load_users():
    if not os.path.exists(USERS_FILE):
        return {}
    with open(USERS_FILE, "r") as f:
        return json.load(f)

def save_users(users):
    with open(USERS_FILE, "w") as f:
        json.dump(users, f, indent=4)

# ------------------ VEHÍCULO ------------------
try:
    vehicle = VehicleController()
    print("Vehicle controller initialized successfully")
except Exception as e:
    print(f"Error initializing vehicle: {e}")
    vehicle = None

# ------------------ CÁMARA ------------------
class Camera:
    def __init__(self):
        self.process = None

    def start(self):
        """Inicia el proceso de la cámara."""
        if self.process is None:
            print("🎥 Iniciando cámara...")
            self.process = subprocess.Popen([
                'libcamera-vid', '-t', '0',
                '--codec', 'mjpeg',
                '--width', '640', '--height', '480',
                '-o', '-', '--nopreview'
            ], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, bufsize=0)

    def stop(self):
        """Detiene la cámara si está activa."""
        if self.process:
            print("🛑 Deteniendo cámara...")
            self.process.terminate()
            self.process.wait(timeout=2)
            self.process = None

    def get_frame(self):
        """Lee y devuelve un frame JPEG completo."""
        if not self.process:
            return None
        while True:
            byte = self.process.stdout.read(1)
            if not byte:
                return None
            if byte == b'\xff':
                next_byte = self.process.stdout.read(1)
                if next_byte == b'\xd8':  # Inicio JPEG
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
    """Genera el stream MJPEG."""
    while True:
        frame = camera.get_frame()
        if frame:
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')

# ------------------ RUTAS DE PÁGINAS ------------------

@main.route("/")
def home():
    if "user" in session:
        return redirect(url_for("main.dashboard"))
    return redirect(url_for("main.login"))

@main.route("/register", methods=["GET", "POST"])
def register():
    if request.method == "POST":
        username = request.form.get("username")
        email = request.form.get("email")
        password = request.form.get("password")

        users = load_users()
        if username in users:
            return render_template("register.html", error="Usuario ya existe")

        users[username] = {
            "email": email,
            "password": generate_password_hash(password)
        }
        save_users(users)
        return redirect(url_for("main.login"))

    return render_template("register.html")

@main.route("/login", methods=["GET", "POST"])
def login():
    if request.method == "POST":
        username = request.form["username"]
        password = request.form["password"]

        users = load_users()
        if username in users and check_password_hash(users[username]["password"], password):
            session["user"] = username
            return redirect(url_for("main.dashboard"))
        else:
            return render_template("login.html", error="Credenciales inválidas")
    return render_template("login.html")

@main.route("/dashboard")
def dashboard():
    if "user" not in session:
        return redirect(url_for("main.login"))
    return render_template("dashboard.html", user=session["user"])

@main.route("/logout")
def logout():
    session.pop("user", None)
    camera.stop()  # Por si estaba activa
    return redirect(url_for("main.login"))

@main.route("/control")
def control():
    """Página de control con cámara y botones del vehículo."""
    if "user" not in session:
        return redirect(url_for("main.login"))

    camera.start()  # Solo aquí se enciende la cámara
    return render_template("control.html", user=session["user"])

# ------------------ APIs ------------------

@main.route("/api/command", methods=["POST"])
def api_command():
    """Control del carrito (usando ejecutar_comando)."""
    if "user" not in session:
        return jsonify({"error": "No autorizado"}), 403

    data = request.get_json()
    comando = data.get("command")
    print("➡️ Comando recibido:", comando)
    ejecutar_comando(comando)
    return jsonify({"status": "ok", "command": comando})

@main.route("/api/control", methods=["POST"])
def api_vehicle_control():
    """Control del vehículo (gpio_wrapper)."""
    if "user" not in session:
        return jsonify({"error": "No autorizado"}), 403

    if not vehicle:
        return jsonify({"error": "Vehicle not initialized"}), 500

    data = request.json
    action = data.get("action")
    speed = data.get("speed", 80)

    result = 0
    if action == "forward":
        result = vehicle.forward(speed)
    elif action == "backward":
        result = vehicle.backward(speed)
    elif action == "left":
        result = vehicle.left(speed)
    elif action == "right":
        result = vehicle.right(speed)
    elif action == "stop":
        result = vehicle.stop()
    else:
        return jsonify({"error": "Invalid action"}), 400

    if result < 0:
        return jsonify({"error": "Command failed"}), 500

    return jsonify({"status": "success", "action": action, "speed": speed})

@main.route("/api/status")
def api_status():
    return jsonify({
        "vehicle_initialized": vehicle is not None,
        "camera_active": camera.process is not None
    })

@main.route("/video_feed")
def video_feed():
    """Stream MJPEG solo disponible si hay sesión."""
    if "user" not in session:
        return redirect(url_for("main.login"))
    return Response(gen_frames(),
                    mimetype="multipart/x-mixed-replace; boundary=frame")
