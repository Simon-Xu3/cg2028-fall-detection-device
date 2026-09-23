# Forward the band's ALERT lines from the serial port to the caregiver's Telegram.
#
#   pip3 install pyserial requests          (once)
#   cp tools/secrets.py.example tools/secrets.py   and fill it in (git-ignored)
#   python3 tools/relay.py                  (Ctrl-C to stop)

import glob
import serial      # talks to the board over USB
import requests    # sends HTTP requests to Telegram
from secrets import BOT_TOKEN, CHAT_ID, HOME_ADDRESS

# What the caregiver reads for each event the band can send
MESSAGES = {
    "FALL":       "FALL DETECTED. The wearer has not pressed OK yet.",
    "LONG_LIE_0": "NO RESPONSE AND NO MOVEMENT FOR 30 s. Call 995 now. Address: " + HOME_ADDRESS,
    "LONG_LIE_1": "NO RESPONSE FOR 30 s. Wearer is moving but cannot get up. Address: " + HOME_ADDRESS,
    "ACK":        "Alert acknowledged on the band.",
}

def send_telegram(text):
    url = "https://api.telegram.org/bot" + BOT_TOKEN + "/sendMessage"
    requests.post(url, data={"chat_id": CHAT_ID, "text": text})

port = glob.glob("/dev/cu.usbmodem*")[0]        # the ST-LINK serial port
board = serial.Serial(port, 115200)
print("relay: reading", port)

while True:
    line = board.readline().decode(errors="ignore").strip()   # e.g. ALERT,LONG_LIE,46961,moved=1
    print(line)

    if not line.startswith("ALERT"):
        continue                                # normal CSV line, nothing to send

    parts = line.split(",")                     # ["ALERT", "LONG_LIE", "46961", "moved=1"]
    event = parts[1]
    if event == "LONG_LIE":
        event = "LONG_LIE_" + parts[3][-1]      # last character of "moved=1" -> "LONG_LIE_1"

    send_telegram(MESSAGES[event])
    print("sent:", event)
