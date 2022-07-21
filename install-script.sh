INITIAL_PATH=$(pwd)

cd ~

wget https://mirrors.edge.kernel.org/pub/linux/bluetooth/bluez-5.64.tar.xz

tar xvf bluez-5.64.tar.xz

sudo apt-get install libusb-dev libdbus-1-dev libglib2.0-dev libudev-dev libical-dev libreadline-dev docutils-common python3-pip libfl2

cd bluez-5.64

export LDFLAGS=-lrt

./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var --enable-library -disable-systemd

make

sudo make install

cd $INITIAL_PATH

pip3 install bleak adafruit-nrfutil

curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=~/.local/bin sh

arduino-cli config init

arduino-cli config set board_manager.additional_urls https://adafruit.github.io/arduino-board-index/package_adafruit_index.json

adafruit-nrfutil version

arduino-cli core install adafruit:nrf52

arduino-cli lib install "Adafruit Arcada Library"

arduino-cli compile -b adafruit:nrf52:cluenrf52840 pihole-monitor/

arduino-cli upload -b adafruit:nrf52:cluenrf52840 -p /dev/ttyACM0 pihole-monitor/

