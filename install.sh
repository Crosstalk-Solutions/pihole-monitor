chmod +x spinner
sudo cp spinner /usr/local/bin/

INITIAL_PATH=$(pwd)

cd ~

echo "Downloading bluez"
wget -q --show-progress https://mirrors.edge.kernel.org/pub/linux/bluetooth/bluez-5.64.tar.xz
echo -e "\xE2\x9C\x94 Downloaded bluez"

echo "Unzipping bluez"
spinner tar xvf bluez-5.64.tar.xz
echo -e "\xE2\x9C\x94 Unzipped bluez"

echo "Installing dependencies"
spinner sudo apt-get install libusb-dev libdbus-1-dev libglib2.0-dev libudev-dev libical-dev libreadline-dev docutils-common python3-pip libfl2 -y
echo -e "\xE2\x9C\x94 Installed dependencies"

cd bluez-5.64
export LDFLAGS=-lrt

echo "Configuring bluez"
spinner ./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var --enable-library -disable-systemd
echo -e "\xE2\x9C\x94 Configured bluez"

echo "Building bluez"
spinner make
echo -e "\xE2\x9C\x94 Built bluez"

echo "Installing bluez"
spinner sudo make install
echo -e "\xE2\x9C\x94 Installed bluez"

cd $INITIAL_PATH

echo "Installing Python dependencies"
spinner pip3 install bleak adafruit-nrfutil
echo -e "\xE2\x9C\x94 Installed Python dependencies"

echo "Adafruit NRFUtil Version"
adafruit-nrfutil version

echo "Installing Arduino CLI"
spinner curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=~/.local/bin sh
echo -e "\xE2\x9C\x94 Installed Arduino CLI"

export PATH="$HOME/.local/bin:$PATH"

echo "Configuring Arduino CLI"
spinner arduino-cli config init
spinner arduino-cli config set board_manager.additional_urls https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
echo -e "\xE2\x9C\x94 Configured Arduino CLI"

echo "Installing Arduino board"
spinner arduino-cli core install adafruit:nrf52
echo -e "\xE2\x9C\x94 Installed Arduino board"

echo "Installing Arduino libraries"
spinner arduino-cli lib install "Adafruit Arcada Library"
echo -e "\xE2\x9C\x94 Installed Arduino libraries"

echo "Compiling sketch"
spinner arduino-cli compile -b adafruit:nrf52:cluenrf52840 pihole-monitor/
echo -e "\xE2\x9C\x94 Compiled sketch"

echo -e "Uploading to CLUE"
arduino-cli upload -b adafruit:nrf52:cluenrf52840 -p /dev/ttyACM0 pihole-monitor/

sleep 5
read -r line < /dev/ttyACM0
echo "Read MAC Address: $line"

sed -i "s/00:00:00:00:00:00/$line/" main.py

echo -e "Installing service"
sed -i "s/USERNAME/${USER}/" pihole-monitor.service
sed -i "s:FILE:${INITIAL_PATH}/main.py:" pihole-monitor.service
sudo cp pihole-monitor.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl start pihole-monitor
sudo systemctl enable pihole-monitor