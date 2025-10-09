# Proyecto1-Empotrados

# Proyecto — Sistema embebido a la medida para el control remoto de un vehículo mediante servidor web

**Curso:** CE-1113 Sistemas Empotrados  
**Instituto Tecnológico de Costa Rica**  
**Profesor:** Dr.-Ing. Jeferson González Gómez  
**Fecha de entrega:** 7 de octubre de 2025  

---


## Tabla de Contenidos

- [Objetivos](#objetivo)
- [Descripción general del sistema](#descripción-general-del-sistema)
- [Características](#características)
- [Requisitos del Sistema](#requisitos-del-sistema)
- [Arquitectura del Sistema](#arquitectura-del-sistema)
- [Instalación y Compilación](#instalación-y-compilación)
- [Documentación de la API (Flask)](#documentación_de_la_API_(Flask))
- [Configuración del Sistema](#configuración-del-sistema)
- [Uso del Sistema](#uso-del-sistema)
- [Troubleshooting](#troubleshooting)

---


## Objetivo

Desarrollar un **sistema embebido a la medida** capaz de controlar remotamente un vehículo autónomo mediante un **servidor web ejecutado sobre una imagen Linux personalizada** construida con **Yocto Project**.  

El sistema permite:
- Controlar el movimiento del vehículo (adelante, atrás, izquierda, derecha) con **PWM**.  
- Controlar luces indicadoras y luces de dirección.  
- Transmitir video en tiempo real desde una cámara.  
- Ejecutar el servidor Flask automáticamente al inicio del sistema.  
- Compilar el software mediante **desarrollo cruzado** con un *toolchain SDK*.  

---

## Descripción general del sistema

Este proyecto integra hardware y software embebido para crear un **vehículo controlado vía WiFi** a través de una interfaz web basada en **Flask**.  
La **Raspberry Pi 4** ejecuta una **imagen mínima de Linux** construida con **Yocto 5.0 “Scarthgap”**, que contiene únicamente los componentes necesarios (Python3, Flask, controladores GPIO y cámara).

El sistema se comunica a través de una **biblioteca dinámica (`libgpioio.so`)** compilada cruzadamente, la cual gestiona motores, LEDs y cámara.

> *El objetivo principal es optimizar los recursos del sistema embebido, manteniendo una arquitectura modular y de bajo consumo.*


## Características

### Funcionalidades Principales
- *Control remoto del vehículo* via interfaz web responsiva
- *Streaming de video en tiempo real* desde cámara CSI
- *Control PWM de velocidad* de motores DC (0-100%)
- *Sistema de iluminación LED* con modo automático/manual
- *Sensor ultrasónico* HC-SR04 para detección de obstáculos
- *Captura de imágenes* desde la interfaz web
- *Sistema de autenticación* con login seguro
- *Inicio automático* del servidor al arranque del sistema

### Especificaciones Técnicas
- Sistema operativo embebido personalizado con *Yocto Project 5.0 (Scarthgap)*
- Servidor web *Flask* en Python 3
- Streaming de video con *FFmpeg* y *V4L2*
- Control de GPIO mediante biblioteca compartida en C
- PWM por software a 1kHz
- Interfaz web moderna con diseño responsivo


---

## Requisitos del Sistema

### Hardware Necesario
| Componente | Especificación |
|------------|----------------|
| *SBC* | Raspberry Pi 4 Model B (2GB+ RAM) |
| *Cámara* | Módulo de cámara Raspberry Pi compatible (CSI) |
| *Motores* | 2x Motores DC con driver L298N |
| *Sensor* | HC-SR04 Ultrasónico |
| *LEDs* | 4x LEDs indicadores |
| *Almacenamiento* | Tarjeta microSD 16GB+ (Clase 10 o superior) |
| *Alimentación* | Batería/Fuente externa para motores (5-12V) |

### Software Necesario (PC de Desarrollo)
- Ubuntu 20.04/22.04 LTS (64-bit)
- Python 3.8+
- Git
- 50GB+ espacio en disco libre
- 8GB+ RAM recomendado

---


## Arquitectura del sistema

### Diagrama de arquitectura del sistema



Figura 1. Diagrama de arquitectura del sistema


Figura 2. Diagrama de arquitectura de Integración de Software y Hardware en Raspberry Pi 4 (Yocto + Flask + GPIO)

---
## Instalación y Compilación

### Instalación de Yocto Project

Antes de crear la imagen de sistema embebido, se debe preparar el entorno host.

#### Requisitos del sistema

```bash
sudo apt-get install gawk wget git diffstat unzip texinfo gcc-multilib \
build-essential chrpath socat cpio python3 python3-pip python3-pexpect \
xz-utils debianutils iputils-ping python3-git python3-jinja2 python3-subunit \
zstd liblz4-tool file locales libacl1
```

### Descarga e instalación de Yocto

La versión utilizada en este proyecto es Yocto 5.0.10 (LTS Scarthgap).

```bash
wget https://downloads.yoctoproject.org/releases/yocto/yocto-5.0.10/poky-ac257900c33754957b2696529682029d997a8f28.tar.bz2

```
Extraer e ingresar al directorio:

```bash
tar -xvf poky-ac257900c33754957b2696529682029d997a8f28.tar.bz2
cd poky
```

### Preparación del entorno de construcción
#### Agregar soporte para Raspberry Pi 4
Clonar el BSP (Board Support Package):

```bash
git clone -b scarthgap https://github.com/agherzan/meta-raspberrypi.git
```

#### Crear un directorio de construcción:
```bash
mkdir rpi4
source oe-init-build-env rpi4
```
#### Configurar conf/local.conf
Editar el archivo 
```bash
nano conf/local.conf
```
Agregar al final:

```bash
MACHINE ?= "raspberrypi4"
INHERIT += "rm_work"
DL_DIR ?= "/home/usuario/poky/downloads"

```

#### Incorporar carpetas del proyecto

Clonar el repositorio del proyecto:
```bash
git clone -b develop https://github.com/kunZhen/Proyecto1-Empotrados.git
```
Copiar las capas y componentes personalizados dentro de poky:

```bash
cp -r Proyecto1-Empotrados/meta-myconfig poky/
cp -r Proyecto1-Empotrados/vehicle-server poky/
cp -r Proyecto1-Empotrados/gpioio poky/
cp -r Proyecto1-Empotrados/servidorweb poky/
```


#### Configurar conf/bblayers.conf
Agregar la capa de soporte de Raspberry Pi:

```bash 
nano conf/local.conf
```
```bash
BBLAYERS ?= " \ 
/home/usuario/poky/meta \
/home/usuario/poky/meta-poky \
/home/usuario/poky/meta-yocto-bsp \
/home/usuario/poky/meta-raspberrypi \
/home/usuario/poky/meta-myconfig \
"
# WiFi
DISTRO_FEATURES:append = " wifi"
IMAGE_INSTALL:append = " linux-firmware-rpidistro-bcm43455 wpa-supplicant iw"

# SSH
DISTRO_FEATURES:append = " ssh"
EXTRA_IMAGE_FEATURES += "ssh-server-openssh"

# Python y servidor web
IMAGE_INSTALL:append = " python3 python3-flask python3-werkzeug"

# Video
VIDEO_CAMERA = "1"
IMAGE_INSTALL:append = " v4l-utils ffmpeg"
RPI_EXTRA_CONFIG += '\n camera_auto_detect=1'
RPI_EXTRA_CONFIG += '\n start_x=1'

# Inicio automático
IMAGE_INSTALL:append = " vehicle-autostart"
```

### Compilar la Biblioteca GPIO

```bash
cd ~/Proyecto1-Empotrados/gpioio

# Configurar cross-compilación
source ~/poky/rpi4/tmp/environment-setup-cortexa7t2hf-neon-vfpv4-poky-linux-gnueabi

# Compilar
make clean
make
```

### Escribir la Imagen en la SD

```bash
cd ~/poky/rpi4/tmp/deploy/images/raspberrypi4

# Identificar la SD (ejemplo: /dev/mmcblk0)
lsblk

# Escribir imagen (CUIDADO: verificar el dispositivo correcto)
sudo dd if=rpi-test-image-raspberrypi4.wic of=/dev/mmcblk0 bs=4M status=progress
sync
```


### Desplegar Aplicaciones

```bash
cd ~/Proyecto1-Empotrados/gpioio

# Desplegar biblioteca y servidor
make deploy-ssh
make deploy-server
```


---


### Ejecución del servidor web

#### Copiar archivos al dispositivo


```bash
scp build/lib/libgpioio.so.1.0.0 root@<IP_RPI>:~
scp build/bin/test_app root@<IP_RPI>:~
```

Iniciar servidor Flask manualmente
```bash
cd /root/vehicle-server
python3 app.py
```


El servicio también se inicia automáticamente gracias a: /etc/systemd/system/vehicle-server.service

## Documentación de la API (Flask)

| Endpoint  | Método | Descripción                            |
|------------|---------|----------------------------------------|
| `/`        | GET     | Página principal del panel de control  |
| `/login`   | POST    | Autenticación de usuario               |
| `/control` | POST    | Control de movimiento                  |
| `/lights`  | POST    | Encendido/apagado de luces             |
| `/video`   | GET     | Streaming de cámara en tiempo real     |



## Uso del Sistema

### Inicio del Sistema

1. *Insertar la SD* en la Raspberry Pi
2. *Conectar alimentación*
3. *Esperar ~60 segundos* para el arranque completo
4. El servidor se inicia automáticamente

### Acceso a la Interfaz Web

1. Conectar a la misma red WiFi que la Raspberry Pi
2. Abrir navegador web
3. Navegar a: http://192.168.100.234:5000/login
4. Ingresar credenciales:
   - Usuario: admin
   - Contraseña: admin123

### Funcionalidades de la Interfaz

#### Control de Movimiento
- *▲ Adelante:* Avanza hacia adelante
- *▼ Atrás:* Retrocede
- *◄ Izquierda:* Giro a la izquierda (motor derecho activo)
- *► Derecha:* Giro a la derecha (motor izquierdo activo)
- *■ Stop:* Detiene todos los motores

#### Control de Velocidad
- Slider de 0% a 100%
- Ajusta el duty cycle del PWM
- Visualización en tiempo real de la señal PWM

#### Sistema de Iluminación
- *Modo Automático:* LEDs siguen el movimiento del vehículo
  - Adelante → LEDs frontales
  - Atrás → LEDs traseros
  - Izquierda → LEDs izquierdos
  - Derecha → LEDs derechos
- *Modo Manual:* Control independiente de cada grupo de LEDs

#### Sensor de Proximidad
- Monitoreo continuo de distancia (actualización cada 200ms)
- Alerta visual cuando detecta obstáculos < 20cm
- Visualización de distancia en centímetros

#### Captura de Imágenes
- Botón "📷 Capturar Imagen"
- Guarda en /root/captures/
- Vista previa inmediata en la interfaz
- Formato: JPEG con timestamp

### Atajos de Teclado

- *W / ↑:* Avanzar
- *S / ↓:* Retroceder
- *A / ←:* Izquierda
- *D / →:* Derecha
- *Espacio:* Stop

### Comandos SSH Útiles

```bash
# Conectar por SSH
ssh root@192.168.100.234

# Ver estado del servidor
/etc/init.d/vehicle-server status

# Reiniciar servidor
/etc/init.d/vehicle-server restart

# Ver logs en tiempo real
tail -f /tmp/vehicle-server.log

# Ver imágenes capturadas
ls -lh /root/captures/

# Descargar capturas
scp root@192.168.100.234:/root/captures/*.jpg ~/
```

---


## Troubleshooting

### Problemas Comunes

#### 1. No se conecta al WiFi

*Síntomas:* Raspberry Pi no obtiene IP, no responde a ping

*Soluciones:*
```bash
# Verificar configuración WiFi
cat /etc/wpa_supplicant/wpa_supplicant-wlan0.conf

# Ver estado de wpa_supplicant
wpa_cli status

# Reiniciar WiFi
ip link set wlan0 down
ip link set wlan0 up
```

#### 2. El servidor no inicia automáticamente

*Síntomas:* No se puede acceder a http://IP:5000

*Soluciones:*
```bash
# Verificar que el script existe
ls -la /etc/init.d/vehicle-server

# Iniciar manualmente
/etc/init.d/vehicle-server start

# Ver logs
tail -100 /tmp/vehicle-server.log
```

#### 3. Los motores no responden

*Síntomas:* Botones de control no mueven el vehículo

*Soluciones:*
```bash
# Verificar biblioteca GPIO
ls -la /root/gpioio/lib/libgpioio.so.1

# Probar GPIO manualmente
echo 18 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio18/direction
echo 1 > /sys/class/gpio/gpio18/value

# Verificar logs del servidor
cat /tmp/vehicle-server.log | grep "Error"
```

#### 4. No aparece el video

*Síntomas:* Cuadro negro en lugar del streaming

*Soluciones:*
```bash
#### Verificar cámara
ls -la /dev/video0

#### Probar captura manual
v4l2-ctl --device=/dev/video0 --set-fmt-video=width=640,height=480,pixelformat=MJPG --stream-mmap --stream-to=test.jpg --stream-count=1

#### Reiniciar servidor
/etc/init.d/vehicle-server restart
```

#### 5. Error al compilar Yocto

*Síntomas:* bitbake falla con errores

*Soluciones:*
```bash
#### Limpiar build
bitbake -c cleanall rpi-test-image

#### Verificar espacio en disco
df -h

#### Verificar dependencias
sudo apt install -y gawk wget git diffstat unzip texinfo gcc build-essential

#### Intentar de nuevo
bitbake rpi-test-image
```