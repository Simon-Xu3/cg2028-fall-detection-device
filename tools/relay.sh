#!/bin/bash
# Forward the band's ALERT lines from the serial port to a Telegram chat.
#
#   1. copy tools/secrets.sh.example to tools/secrets.sh and fill in the bot token + chat id
#   2. ./tools/relay.sh                      (auto-detects the ST-LINK port)
#      ./tools/relay.sh /dev/cu.usbmodem1234 (or give the port yourself)
#
# The board prints one line per event:  ALERT,FALL,<ms>,moved=0
#                                       ALERT,LONG_LIE,<ms>,moved=1
#                                       ALERT,ACK

source "$(dirname "$0")/secrets.sh" || exit 1

PORT=${1:-$(ls /dev/cu.usbmodem* | head -1)}
echo "relay: reading $PORT, sending to chat $CHAT_ID"

send() {
    curl -s "https://api.telegram.org/bot$BOT_TOKEN/sendMessage" \
         -d chat_id="$CHAT_ID" -d text="$1" > /dev/null
}

exec 3<>"$PORT"
stty -f "$PORT" 115200 raw

cat <&3 | grep --line-buffered '^ALERT' | while IFS=, read -r _ event tick moved; do
    case "$event" in
        FALL)     send "FALL DETECTED. The wearer has not pressed OK yet." ;;
        LONG_LIE) if [ "$moved" = "moved=0" ]; then
                      send "NO RESPONSE AND NO MOVEMENT FOR 30 s. Call an ambulance."
                  else
                      send "NO RESPONSE FOR 30 s. Wearer is moving but cannot get up."
                  fi ;;
        ACK)      send "Alert acknowledged on the band." ;;
    esac
    echo "sent: $event $moved"
done
