# Adafruit CLUE Pi-Hole Monitor

This is a simple desk toy / monitor for a [Pi-Hole](https://pi-hole.net) installation. 


## Installation

1. [Install Pi-Hole](https://github.com/pi-hole/pi-hole/#one-step-automated-install). The installation script for this Pi-Hole Monitor has been tested on [Raspberry Pi OS (64-bit) Lite](https://www.raspberrypi.com/software/operating-systems/#raspberry-pi-os-64-bit). Other versions may work but have not been validated.
2. Clone this repository! You do not need to be root.
```sh
git clone https://github.com/crosstalk-solutions/pihole-monitor.git
```
3. Enter the repository folder
```sh
cd pihole-monitor
```
4. Plug in your CLUE! Connect it (with a USB cable) to your Raspberry Pi. The install script will automatically compile and upload firmware to the CLUE.
5. Run the install script
```sh
./install.sh
```