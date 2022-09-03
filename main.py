import asyncio
import sys
import urllib.request
import json
import time
import threading
from bleak import BleakScanner, BleakClient
from bleak.backends.scanner import AdvertisementData
from bleak.backends.device import BLEDevice

UART_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
UART_RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
UART_TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

UART_SAFE_SIZE = 20

f = open("/etc/pihole/setupVars.conf", "r")
conf = f.read()
f.close()
WEBPASSWORD = conf.split("WEBPASSWORD=")[1].split("\n")[0]


def chunks(lst, n):
    for i in range(0, len(lst), n):
        yield lst[i:i + n]


async def uart_terminal():

    def match_nus_uuid(device: BLEDevice, adv: AdvertisementData):
        if UART_SERVICE_UUID.lower() in adv.service_uuids:
            return True

        return False

    device = "00:00:00:00:00:00"

    def handle_disconnect(_: BleakClient):
        print("Device was disconnected, goodbye.")
        for task in asyncio.all_tasks():
            task.cancel()

    def handle_rx(_: int, data: bytearray):
        print("received:", data)
        if data == b'1':
            urllib.request.urlopen("http://localhost/admin/api.php?disable=300&auth=" + WEBPASSWORD)

    async with BleakClient(device, disconnected_callback=handle_disconnect) as client:
        await client.start_notify(UART_TX_CHAR_UUID, handle_rx)

        print("Connected...")

        loop = asyncio.get_running_loop()
        prev_time = time.time()

        while True:
            small = dict()
            fetched = urllib.request.urlopen("http://localhost/admin/api.php?summary").read()
            parse = json.loads(fetched)
            small["dbb"] = parse["domains_being_blocked"]
            small["dqt"] = parse["dns_queries_today"]
            small["abt"] = parse["ads_blocked_today"]
            small["apt"] = parse["ads_percentage_today"]
            fetched = urllib.request.urlopen("http://localhost/admin/api.php?overTimeData10mins").read()
            parse = json.loads(fetched)
            domains_over_time = list(parse['domains_over_time'].values())
            ads_over_time = list(parse['ads_over_time'].values())
            small['dot'] = domains_over_time
            small['aot'] = ads_over_time
            data = json.dumps(small, separators=(',', ':'))
            data += '\n'
            data = str.encode(data)
            if not data:
                print("no data")
                break

            data = data.replace(b"\n", b"\r\n")

            data = chunks(data, UART_SAFE_SIZE)

            for chunk in data:
                await client.write_gatt_char(UART_RX_CHAR_UUID, chunk)

            print("sent")

            time.sleep(10)


if __name__ == "__main__":
    try:
        asyncio.run(uart_terminal())
    except asyncio.CancelledError:
        pass
