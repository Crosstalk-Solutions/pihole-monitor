wget https://mirrors.edge.kernel.org/pub/linux/bluetooth/bluez-5.64.tar.xz

tar xvf bluez-5.64.tar.xz

sudo apt-get install libusb-dev libdbus-1-dev libglib2.0-dev libudev-dev libical-dev libreadline-dev

cd bluez-5.64

export LDFLAGS=-lrt


sudo apt-get install docutils-common

./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var --enable-library -disable-systemd

make

sudo make install

sudo apt install python3-pip

pip3 install bleak