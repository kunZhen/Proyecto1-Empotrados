FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://wpa_supplicant.conf-sane \
            file://rfkill-unblock.service"

do_install:append() {
    install -d ${D}${sysconfdir}/wpa_supplicant/
    install -m 600 ${WORKDIR}/wpa_supplicant.conf-sane ${D}${sysconfdir}/wpa_supplicant/wpa_supplicant-wlan0.conf
    
    # Habilita wpa_supplicant@wlan0.service
    install -d ${D}${sysconfdir}/systemd/system/multi-user.target.wants
    ln -sf ${systemd_system_unitdir}/wpa_supplicant@.service \
        ${D}${sysconfdir}/systemd/system/multi-user.target.wants/wpa_supplicant@wlan0.service
    
    # Instala y habilita servicio rfkill-unblock
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/rfkill-unblock.service ${D}${systemd_system_unitdir}/
    
    ln -sf ${systemd_system_unitdir}/rfkill-unblock.service \
        ${D}${sysconfdir}/systemd/system/multi-user.target.wants/rfkill-unblock.service
}

FILES:${PN} += "${sysconfdir}/wpa_supplicant/wpa_supplicant-wlan0.conf \
                ${sysconfdir}/systemd/system/multi-user.target.wants/wpa_supplicant@wlan0.service \
                ${systemd_system_unitdir}/rfkill-unblock.service \
                ${sysconfdir}/systemd/system/multi-user.target.wants/rfkill-unblock.service"
