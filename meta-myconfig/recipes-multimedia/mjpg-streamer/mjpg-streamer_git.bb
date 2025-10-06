SUMMARY = "MJPG Streamer"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://LICENSE;md5=751419260aa954499f7abaabaa882bbe"

DEPENDS = "jpeg"

SRC_URI = "git://github.com/jacksonliam/mjpg-streamer.git;protocol=https;branch=master"
SRCREV = "310b29f4a94c46652b20c4b7b6e5cf24e532af39"

S = "${WORKDIR}/git/mjpg-streamer-experimental"

inherit cmake

do_install() {
    install -d ${D}${bindir}
    install -d ${D}${libdir}/mjpg-streamer
    install -d ${D}${datadir}/mjpg-streamer/www
    
    # Instala el ejecutable
    if [ -f ${B}/mjpg_streamer ]; then
        install -m 0755 ${B}/mjpg_streamer ${D}${bindir}/
    fi
    
    # Instala los plugins .so (pueden estar en subdirectorios)
    find ${B} -name "input_*.so" -exec install -m 0644 {} ${D}${libdir}/mjpg-streamer/ \;
    find ${B} -name "output_*.so" -exec install -m 0644 {} ${D}${libdir}/mjpg-streamer/ \;
    
    # Instala archivos web si existen
    if [ -d ${S}/www ]; then
        cp -r ${S}/www/* ${D}${datadir}/mjpg-streamer/www/
    fi
}

FILES:${PN} += "${libdir}/mjpg-streamer/*.so ${datadir}/mjpg-streamer"

INSANE_SKIP:${PN} += "ldflags"
