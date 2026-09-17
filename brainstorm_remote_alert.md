# Remote alert / "auto call ambulance" options (brainstorm, 16 Sep)

Goal from Norman: after a confirmed fall the band must (1) make noise so people nearby help,
(2) tell a caregiver's phone automatically, (3) escalate to "call an ambulance" if nobody
responds. Constraint: the ONLY external part we may take is the Grove buzzer. Everything
else must already be on the Discovery board.

## What the board already has for this (no extra parts)
| Radio | Chip | Driver situation | Verdict |
|---|---|---|---|
| Wi-Fi 2.4 GHz | ISM43362 (Inventek), SPI3 + PE1 data-ready | `es_wifi.c` is ALREADY inside the starter zip (Drivers/BSP/Components/es_wifi, 2076 lines, not excluded from the build). The SPI glue `es_wifi_io.c` + `wifi.c` is in the lab example on disk: Lab/workspace/WiFi_HTTP_Server/Common/. That example is ST's official app, not a student project, so reusing it is fine. | USE (gated, see below) |
| BLE | SPBTLE-RF, SPI3 | no driver in starter, needs BlueNRG middleware (10k+ lines) | reject |
| NFC | ST25DV, I2C2 | BSP present, but a phone must be tapped on the band = not an automatic alert | reject for alerts |

Lecturer hint: the assignment page lists "wireless" as an enhancement category and the
starter ships with the Wi-Fi driver. Some groups are expected to use it.

## What "call an ambulance" can honestly mean
We cannot dial 995 from a dev board. What we can do, and say so on the slide:
1. Buzzer plays Morse SOS (··· --- ···) + LED solid, so passers-by recognise an emergency.
2. Telegram message to the caregiver's phone within seconds of the FALL state.
3. If nobody presses the band's button or answers on Telegram before the Long-Lie timer,
   a second message "NO RESPONSE - CALL AMBULANCE 995 NOW" goes to an emergency group chat
   and repeats every 30 s until acknowledged.
Slide wording: "In deployment step 3 would call a telco/SMS API; in this project the
escalation message is the ambulance call."

## Three ways to reach the phone (all end in the same Telegram bot)
| Path | Board side | Laptop side | Board code | Risk | What the GA sees |
|---|---|---|---|---|---|
| G  UART gateway | prints `SOS,FALL,<tick>,<peak_acc>,<peak_gyro>` on state entry, receives `a` (ack) | `relay.py --serial`: reads the line, calls Telegram sendMessage, writes `a` back when caregiver taps the "I'm on it" button | 0 new (we already print) | none | phone buzzes, but the laptop is clearly in the path |
| W1 Wi-Fi to laptop relay (plain HTTP) | board joins phone hotspot, on state entry opens TCP to laptop:8080, sends `POST /alert` with the same line, closes | `relay.py --http`: tiny HTTP server, forwards to Telegram, answers `GET /ack` | ~150 lines: wifi_alert.c (init, send) + it.c handlers + SystemClock_Config copied from the ST example | medium: hotspot must be 2.4 GHz; first bring-up 1 evening | the band itself is wireless; laptop only bridges to the internet |
| W2 Wi-Fi direct to Telegram (TLS) | same as W1 but socket type SSL to api.telegram.org:443, HTTPS GET /bot<token>/sendMessage | none needed | W1 + ~30 lines + a 1-line driver patch: es_wifi.c line ~1438 only sends the remote IP for plain TCP, SSL must be added to that `if` | HIGH-ish: TLS on this module firmware is unverified by us; handshake 2-5 s blocking; bot token lives on the board | no laptop at all: band -> phone |

Timing cost inside the 20 ms loop:
- G: one 40-byte line = 3.5 ms at 115200. Nothing else.
- W1: open+send+close to a LAN host ~150-400 ms, once per state entry.
- W2: DNS + TLS + request ~2-5 s, once per state entry.
All happen AFTER the fall decision (state entry), never inside SUSPECT/CONFIRMING. During
the send we force the buzzer ON so the alarm never goes silent, and the overrun counter
records exactly one long loop per alert, which we show as evidence.

## Decision
- G is built in all cases (day 2, 2 h). It is the fallback AND the caregiver console.
- W1 is the target for the "wireless" headline. Partner starts it on day 2-3 in a COPY of
  the lab example, independent of the core, so the go/no-go comes by 22 Sep.
- W2 is a 2-3 h spike after W1 works. Keep it only if `getMe` over TLS returns 200 on the
  first evening; otherwise drop it without regret. Selected by one `#define ALERT_PATH`.
- Ack from the phone: G gets it for free (`a` over UART). W1: board polls `GET /ack` every
  3 s with a 300 ms timeout, only while in FALL/LONG_LIE. W2: no phone ack (button only).

## Demo network
NUS Wi-Fi is WPA2-Enterprise, the module cannot join it. Use a phone hotspot, 2.4 GHz
("Maximise Compatibility" on iPhone). Laptop + board both on the hotspot. Print the SSID and
IP on the UART at boot so a failed join is obvious in 10 s, then continue in UART-only mode.
Test the exact hotspot in the exact lab room before the demo day.

## Secrets
- Bot token stays in relay.py on the laptop for G and W1. Only W2 puts it on the board.
- Hotspot SSID/password and (W2) token go in `Core/Inc/secrets.h`, which is git-ignored;
  commit `secrets_template.h`. The Canvas zip DOES include the real secrets.h so the GA's
  build behaves exactly like ours (they cannot join our hotspot anyway; the board then
  prints "WIFI: not joined, UART alerts only" and keeps running).
