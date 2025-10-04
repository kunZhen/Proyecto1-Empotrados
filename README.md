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

Para revisar la tarjeta memoria
```bash
lsblk
```

Desmontar particiones
```bash
sudo umount /dev/sdb1
sudo umount /dev/sdb2
```

```bash
sudo dd if=rpi-test-image-raspberrypi4.rootfs.wic of=/dev/sdb bs=1M status=progress
sync
```

Ambiente de compilación 
```bash
. /opt/poky/5.0.12/environment-setup-cortexa7t2hf-neon-vfpv4-poky-linux-gnueabi
```