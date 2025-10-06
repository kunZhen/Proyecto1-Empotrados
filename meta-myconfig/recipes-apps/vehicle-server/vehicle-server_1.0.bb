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
