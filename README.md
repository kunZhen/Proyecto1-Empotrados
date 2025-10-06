# Proyecto1-Empotrados


```bash
nmap -sn 192.168.100.0/24
```

```bash
scp build/lib/libgpioio.so.1.0.0 root@192.168.100.225:~
```

```bash
scp build/bin/test_app root@192.168.100.225:~
```

## Copiar imagen a memoria uSD


cd ~/poky/
source oe-init-build-env rpi4
bitbake rpi-test-image


Para revisar la tarjeta memoria
```bash
lsblk
```

source oe-init-build-env rpi4

Desmontar particiones
```bash
sudo umount /dev/sdb1
sudo umount /dev/sdb2
```

```bash
sudo dd if=tmp/deploy/images/raspberrypi4/rpi-test-image-raspberrypi4.rootfs.wic of=/dev/sdb bs=1M status=progress
sync
```

Ambiente de compilación 
```bash
. /opt/poky/5.0.12/environment-setup-cortexa7t2hf-neon-vfpv4-poky-linux-gnueabi
```

Modificar archivo de configuración WiFi
```bash
nano meta-myconfig/recipes-connectivity/wpa-supplicant/files/wpa_supplicant.conf-sane
```

Buscar mi ip
```bash
hostname -I
```

```bash
sudo nmap -sn 192.168.100.0/24
```

-----------------------------

## WiFi
```bash
nano ~/poky/meta-myconfig/recipes-connectivity/wpa-supplicant/files/wpa_supplicant.conf-sane
```

## Server
cd /root/vehicle-server
python3 app.py

----------------------------Nueva cámara-----------------------------
# Con v4l2 (driver legacy)
v4l2-ctl --device=/dev/video0 --set-fmt-video=width=640,height=480,pixelformat=MJPG
v4l2-ctl --device=/dev/video0 --stream-mmap --stream-to=test.jpg --stream-count=1

# Verifica que se creó la imagen
ls -lh test.jpg

## Copiar imagen a mi pc
scp root@192.168.100.234:test.jpg ~/

-----------------------
# Receta del servicio
```bash
mkdir -p ~/poky/meta-myconfig/recipes-apps/vehicle-server/files
```

crear archivo de servicio
```bash
nano ~/poky/meta-myconfig/recipes-apps/vehicle-server/files/vehicle-server.service
```

```bash
[Unit]
Description=Vehicle Control Web Server
After=network.target wifi-autostart.service

[Service]
Type=simple
User=root
WorkingDirectory=/root/vehicle-server
ExecStart=/usr/bin/python3 /root/vehicle-server/app.py
Restart=always
RestartSec=5
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```

crear receta de instalación
```bash
nano ~/poky/meta-myconfig/recipes-apps/vehicle-server/vehicle-server_1.0.bb
```

```bash
SUMMARY = "Vehicle Control Web Server"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://vehicle-server.service \
           file://app.py \
           file://gpio_wrapper.py \
           file://templates/index.html \
           file://templates/login.html \
           file://static/style.css \
           file://static/control.js"

S = "${WORKDIR}"

inherit systemd

SYSTEMD_SERVICE:${PN} = "vehicle-server.service"
SYSTEMD_AUTO_ENABLE = "enable"

RDEPENDS:${PN} = "python3 python3-flask"

do_install() {
    # Instala archivos del servidor
    install -d ${D}/root/vehicle-server
    install -d ${D}/root/vehicle-server/templates
    install -d ${D}/root/vehicle-server/static
    
    install -m 0644 ${WORKDIR}/app.py ${D}/root/vehicle-server/
    install -m 0644 ${WORKDIR}/gpio_wrapper.py ${D}/root/vehicle-server/
    install -m 0644 ${WORKDIR}/templates/index.html ${D}/root/vehicle-server/templates/
    install -m 0644 ${WORKDIR}/templates/login.html ${D}/root/vehicle-server/templates/
    install -m 0644 ${WORKDIR}/static/style.css ${D}/root/vehicle-server/static/
    install -m 0644 ${WORKDIR}/static/control.js ${D}/root/vehicle-server/static/
    
    # Instala servicio systemd
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/vehicle-server.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} += "/root/vehicle-server"
```

copiar archivos a la receta
```bash
cp ~/Proyecto1-Empotrados/vehicle-server/app.py ~/poky/meta-myconfig/recipes-apps/vehicle-server/files/
cp ~/Proyecto1-Empotrados/vehicle-server/gpio_wrapper.py ~/poky/meta-myconfig/recipes-apps/vehicle-server/files/

mkdir -p ~/poky/meta-myconfig/recipes-apps/vehicle-server/files/templates
cp ~/Proyecto1-Empotrados/vehicle-server/templates/*.html ~/poky/meta-myconfig/recipes-apps/vehicle-server/files/templates/

mkdir -p ~/poky/meta-myconfig/recipes-apps/vehicle-server/files/static
cp ~/Proyecto1-Empotrados/vehicle-server/static/*.css ~/poky/meta-myconfig/recipes-apps/vehicle-server/files/static/
cp ~/Proyecto1-Empotrados/vehicle-server/static/*.js ~/poky/meta-myconfig/recipes-apps/vehicle-server/files/static/
```

## Copiar biblioteca compilada
```bash
mkdir -p ~/poky/meta-myconfig/recipes-support/gpioio/files
cp ~/Proyecto1-Empotrados/gpioio/build/lib/libgpioio.so.1 ~/poky/meta-myconfig/recipes-support/gpioio/files/
```