import ctypes
import os

lib_path = os.path.join(os.path.dirname(__file__), "libcontrol.so")

try:
    control_lib = ctypes.CDLL(lib_path)
    print("Biblioteca C cargada correctamente")
except OSError:
    print("No se pudo cargar libcontrol.so. Modo simulado.")
    control_lib = None

def ejecutar_comando(comando: str):
    if not control_lib:
        print(f"[Simulado] Ejecutando comando: {comando}")
        return

    # Aquí enlazas las funciones reales de la librería C
    if comando == "forward":
        control_lib.move_forward()
    elif comando == "backward":
        control_lib.move_backward()
    elif comando == "left":
        control_lib.turn_left()
    elif comando == "right":
        control_lib.turn_right()
    elif comando == "stop":
        control_lib.stop()
    elif comando == "lights_on":
        control_lib.lights_on()
    elif comando == "lights_off":
        control_lib.lights_off()
    else:
        print(f"Comando desconocido: {comando}")
