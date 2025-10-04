from flask import Flask
from app.routes import main

app = Flask(__name__)
app.secret_key = "supersecret"  # cámbiala

app.register_blueprint(main)

if __name__ == "__main__":
    print(">>> Servidor Flask iniciado en http://0.0.0.0:5000")
    app.run(host="0.0.0.0", port=5000, debug=True)
