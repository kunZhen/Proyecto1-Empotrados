from flask import Flask

def create_app():
    app = Flask(__name__)
    app.secret_key = "cambia-esto-en-produccion"  # requerido para sesiones/login

    # Ruta mínima para probar el servidor
    @app.get("/")
    def hello():
        return "Hola Flask (mínimo). Servidor OK."

    return app
