## Raspberry Pi for running the Hero Display

### Prepare OS

Download the latest Raspbian and write the image to the SD card.

Eject the SD card and reinsert it. You should now see a partition called boot.

Add a file called `wpa_supplicant.conf` with your network info on the
boot partition.
```
network={
   ssid="my_ssid"
   psk="supersekrit"
   key_mgmt=WPA-PSK
}
```

Insert the SD card to the Raspberry Pi and apply power.

After a minute you should be able to `ping raspberrypi.local`

Copy you SSH key to the Pi `ssh-copy-id pi@raspberrypi.local`

Log in to the Pi remotely `ssh pi@raspberrypi.local`

Run `sudo raspi-config`

- Change the password to the same password as the Maker Faire 2017 Particle user.
- Change the hostname to `hero`

Exit raspi-config and reboot.

### Enable CAN driver

Edit `/boot/config.txt` and add these lines at the end

```
# Load CAN driver
dtparam=spi=on
dtoverlay=mcp2515-can0-overlay,oscillator=16000000,interrupt=25
dtoverlay=spi-bcm2835-overlay
```

Install CAN utils `sudo apt install can-utils`

CAN utils are user applications that use the Linux socketcan interface
to send, receive and dump CAN message.

### Set static IP on network card

```
sudo vim /etc/dhcpcd.conf
```

Paste at the end:
```
interface eth0
static ip_address=192.168.1.1/24
```


