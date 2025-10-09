SUMMARY = "Vehicle Server Autostart"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://vehicle-server"

S = "${WORKDIR}"

inherit update-rc.d

INITSCRIPT_NAME = "vehicle-server"
INITSCRIPT_PARAMS = "start 99 2 3 4 5 . stop 20 0 1 6 ."

do_install() {
    install -d ${D}${sysconfdir}/init.d
    install -m 0755 ${WORKDIR}/vehicle-server ${D}${sysconfdir}/init.d/
}
