from flask import Flask, request, jsonify
from flask_cors import CORS

app = Flask(__name__)
CORS(app)  # habilita CORS

@app.route("/api/command", methods=["POST"])
def command():
    data = request.get_json()
    comando = data.get("command", "ninguno")
    print("Comando recibido:", comando)
    return jsonify({"status": "ok", "command": comando})

if __name__ == "__main__":
    app.run(port=5001, debug=True)
