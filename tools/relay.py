# Forward the band's ALERT lines from the serial port to the caregiver's Telegram.
#
#   pip3 install pyserial requests          (once)
#   cp tools/secrets.py.example tools/secrets.py   and fill it in (git-ignored)
#   python3 tools/relay.py                  (Ctrl-C to stop)

import glob
import serial      # talks to the board over USB
import requests    # sends HTTP requests to Telegram
from secrets import BOT_TOKEN, CHAT_ID, HOME_ADDRESS   # private, in the git-ignored secrets.py

# What the caregiver reads for each event the band can send.
# The band has no GPS, so the home address is configured once per wearer.
MESSAGES = {
    "FALL":       "FALL DETECTED. The wearer has not pressed OK yet.",
    "LONG_LIE_0": "NO RESPONSE AND NO MOVEMENT FOR 30 s. Call 995 now. Address: " + HOME_ADDRESS,
    "LONG_LIE_1": "NO RESPONSE FOR 30 s. Wearer is moving but cannot get up. Address: " + HOME_ADDRESS,
    "ACK":        "Alert acknowledged on the band.",
}

def send_telegram(text):
    # One HTTP POST to the Telegram Bot API: who to send to, and what
    url = "https://api.telegram.org/bot" + BOT_TOKEN + "/sendMessage"
    requests.post(url, data={"chat_id": CHAT_ID, "text": text})

port = glob.glob("/dev/cu.usbmodem*")[0]        # the ST-LINK serial port
board = serial.Serial(port, 115200)             # same baud rate as the firmware
print("relay: reading", port)

# Read the board forever. Normal CSV lines are only shown; ALERT lines are sent.
while True:
    line = board.readline().decode(errors="ignore").strip()   # e.g. ALERT,LONG_LIE,46961,moved=1
    print(line)

    if not line.startswith("ALERT"):
        continue                                # normal CSV line, nothing to send

    parts = line.split(",")                     # ["ALERT", "LONG_LIE", "46961", "moved=1"]
    event = parts[1]
    if event == "LONG_LIE":
        event = "LONG_LIE_" + parts[3][-1]      # last character of "moved=1" -> "LONG_LIE_1"

    send_telegram(MESSAGES[event])              # look up the text and send it
    print("sent:", event)
