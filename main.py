"""
UART Service
-------------
An example showing how to write a simple program using the Nordic Semiconductor
(nRF) UART service.
"""

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

# All BLE devices have MTU of at least 23. Subtracting 3 bytes overhead, we can
# safely send 20 bytes at a time to any device supporting this service.
UART_SAFE_SIZE = 20


def chunks(lst, n):
    """Yield successive n-sized chunks from lst."""
    for i in range(0, len(lst), n):
        yield lst[i:i + n]


async def uart_terminal():
    """This is a simple "terminal" program that uses the Nordic Semiconductor
    (nRF) UART service. It reads from stdin and sends each line of data to the
    remote device. Any data received from the device is printed to stdout.
    """

    def match_nus_uuid(device: BLEDevice, adv: AdvertisementData):
        # This assumes that the device includes the UART service UUID in the
        # advertising data. This test may need to be adjusted depending on the
        # actual advertising data supplied by the device.
        if UART_SERVICE_UUID.lower() in adv.service_uuids:
            return True

        return False

    device = "00:00:00:00:00:00"

    def handle_disconnect(_: BleakClient):
        print("Device was disconnected, goodbye.")
        # cancelling all tasks effectively ends the program
        for task in asyncio.all_tasks():
            task.cancel()

    def handle_rx(_: int, data: bytearray):
        print("received:", data)

    async with BleakClient(device, disconnected_callback=handle_disconnect) as client:
        await client.start_notify(UART_TX_CHAR_UUID, handle_rx)

        print("Connected...")

        loop = asyncio.get_running_loop()
        prev_time = time.time()

        while True:
            # This waits until you type a line and press ENTER.
            # A real terminal program might put stdin in raw mode so that things
            # like CTRL+C get passed to the remote device.
            if time.time() - 10 > prev_time:
                prev_time = time.time()
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
                # data will be empty on EOF (e.g. CTRL+D on *nix)
                if not data:
                    print("no data")
                    break

                # some devices, like devices running MicroPython, expect Windows
                # line endings (uncomment line below if needed)
                data = data.replace(b"\n", b"\r\n")

                data = chunks(data, UART_SAFE_SIZE)

                for chunk in data:
                    await client.write_gatt_char(UART_RX_CHAR_UUID, chunk)

                print("sent")

            data = await client.read_gatt_char(UART_TX_CHAR_UUID)
            print("received:", data)
            time.sleep(1)


if __name__ == "__main__":
    try:
        asyncio.run(uart_terminal())
    except asyncio.CancelledError:
        # task is cancelled on disconnect, so we ignore this error
        pass
