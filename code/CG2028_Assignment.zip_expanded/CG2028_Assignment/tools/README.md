# Telegram relay

This laptop program forwards the board's serial `ALERT` messages to a Telegram
chat. It runs separately from the STM32 firmware; the board does not contact
Telegram directly. A USB connection and laptop internet access are required.

## Setup

Run these commands from the `CG2028_Assignment` project directory (the directory
containing `Core`, `Drivers`, and `tools`):

```bash
python3 -m pip install -r tools/requirements.txt
```

Copy `tools/secrets.py.example` to `tools/secrets.py` and enter your own bot token,
chat ID, and wearer address. On macOS/Linux:

```bash
cp tools/secrets.py.example tools/secrets.py
```

On Windows, copy and rename the file using File Explorer. Use `python` instead
of `python3` if that is the name of your Python installation.

## Run

Flash the main firmware, connect the board by USB, and close any serial monitor
that is using the same port. Then run:

```bash
python3 tools/relay.py
```

If automatic port detection fails, supply the ST-LINK serial port explicitly:

```bash
python3 tools/relay.py COM5
python3 tools/relay.py /dev/cu.usbmodemXXXX
```

Use the actual port for your computer. The relay uses 115200 baud. Normal CSV
lines are printed locally; FALL, LONG_LIE (with or without movement), and ACK
alerts are forwarded to Telegram. Press Ctrl-C to stop.

## Submission

Include `relay.py`, `requirements.txt`, this guide, and `secrets.py.example`.
Do not include your populated `secrets.py`. Git ignores it, but STM32CubeIDE's
Archive File export does not necessarily follow Git ignore rules: deselect it
in the export dialog if you have created it. The recipient supplies their own
Telegram configuration.
