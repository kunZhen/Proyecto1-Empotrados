SUMMARY = "GPIO Library for Raspberry Pi"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://libgpioio.so.1"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${libdir}
    install -m 0755 ${WORKDIR}/libgpioio.so.1 ${D}${libdir}/
}

FILES:${PN} = "${libdir}/libgpioio.so.1"
