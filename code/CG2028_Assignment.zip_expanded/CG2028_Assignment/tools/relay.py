# Forward the band's ALERT lines from the serial port to the caregiver's Telegram.
#
# Run from the CG2028_Assignment project directory. See tools/README.md.
#   python3 -m pip install -r tools/requirements.txt   (once)
#   cp tools/secrets.py.example tools/secrets.py   and fill it in (git-ignored)
#   python3 tools/relay.py                  (Ctrl-C to stop)
#   python3 tools/relay.py COM5             (Windows: give the port yourself if needed)

import sys
import serial      # talks to the board over USB
from serial.tools import list_ports   # lists the serial ports on this computer
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
    # One HTTP POST to the Telegram Bot API: who to send to, and what.
    # If the laptop has no internet, give up after 5 s and keep reading the board.
    url = "https://api.telegram.org/bot" + BOT_TOKEN + "/sendMessage"
    try:
        requests.post(url, data={"chat_id": CHAT_ID, "text": text}, timeout=5)
    except requests.RequestException as e:
        print("telegram failed:", e)

# Find the ST-LINK serial port: give it on the command line (COM5, /dev/ttyACM0, ...)
# or let the script pick the first port that looks like an ST-LINK / USB modem.
if len(sys.argv) > 1:
    port = sys.argv[1]
else:
    ports = list_ports.comports()
    match = [p.device for p in ports if "STLink" in (p.description or "") or "STM" in (p.description or "")
             or "usbmodem" in p.device or "ttyACM" in p.device]
    if not match:
        sys.exit("no ST-LINK port found; plug in the board or run: python3 tools/relay.py <port>")
    port = match[0]
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
