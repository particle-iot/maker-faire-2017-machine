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

Edit `/boot/cmdline.txt` and add `usbhid.mousepoll=0` to fix the laggy mouse in the GUI.

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

### Install particle-agent

```
bash <( curl -sL https://particle.io/install-pi )
```

Log in with software+mf2017@particle.io

Call the device `hero`

### Setup web server

```
sudo apt install nginx
```

Edit `/etc/nginx/sites-available/default` to:
```
root /home/pi/website
```

```
sudo service nginx restart
```

### Flash firmware

```
make compile
# outputs firmware.bin
```

Copy firmware to Pi
```
scp firmware.bin pi@hero.lan:
```

Replace firmware (local OTA...)
```
ssh pi@hero.lan
sudo cp firmware.bin /var/lib/particle/devices/7ab72efbc4b6719da784073f/output.bin
particle-agent restart
```

### Start browser on boot

Set the browser home page to http://localhost

Install mouse cursor hiding package
```
sudo apt install unclutter
```

Edit `~/.config/lxsession/LXDE-pi/autostart`

Add
```
#Disable screensaver:
@xset s noblank
@xset s off
@xset -dpms
#Start browser on boot
@chromium-browser --app=http://localhost --start-fullscreen
#Hide mouse pointer
@unclutter -idle 1 -root
```

### Download animation

Generate an ssh key on the Pi
```
ssh-keygen
```

Add the `~/.ssh/id_rsa.pub` to the [Maker Faire Machine repo](https://github.com/spark/maker-faire-2017-machine/settings/keys) with read/write permissions.

Clone the repo to the Pi
```
git clone git@github.com:spark/maker-faire-2017-machine.git
```

