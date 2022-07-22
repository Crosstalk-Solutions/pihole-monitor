arduino-cli compile -b adafruit:nrf52:cluenrf52840 pihole-monitor/
arduino-cli upload -b adafruit:nrf52:cluenrf52840 -p /dev/ttyACM0 pihole-monitor/
sleep 5
read -r line < /dev/ttyACM0
echo "Read MAC Address: $line"