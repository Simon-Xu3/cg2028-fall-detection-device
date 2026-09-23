# Enhancement plan (the optional 9 marks)

Status: 20 Sep 2026. Core detection is done and tested on the board (PR #4).
Everything below is extra. Deadline is 30 Sep 08:00, so code freeze is 26 Sep;
27-29 Sep is slides, report and rehearsal only.

## What "enhancement" means in the rubric

Three rows, 0 / 1 / 3 marks each:

| Row | 3-mark wording (quoted from the rubric) |
|---|---|
| Relevance to an elderly user | "Long Lie escalation, SOS, inactivity monitoring, alerting, or logging" |
| Technical depth | "state-machine extension, adaptive filtering, sensor fusion, communication, logging" |
| Integration and robustness | "does not degrade compulsory fall detection and remains stable throughout the demonstration" |

So the story is: **detect -> alert nearby (buzzer) -> alert caregiver (phone) -> escalate if nobody responds (Long Lie).**
Row 3 is the one that loses marks: a flaky extra feature costs more than no feature.

## Scope (trimmed, agreed to keep it demo-safe)

| # | Feature | Who | Files | Rubric words |
|---|---|---|---|---|
| E1 | `STATE_LONG_LIE`: in FALL, no button press and still lying for `LONG_LIE_MS` (30 s) -> escalate | B (Norman) | fall_detector.c/.h, main.c | state-machine extension, Long Lie escalation |
| E2 | Grove buzzer: Morse SOS in FALL, faster SOS in LONG_LIE, one chirp on ack | A (Ruijie) | new buzzer.c/.h, main.c (3 lines) | SOS, alerting |
| E3 | Telegram alert: board prints one `ALERT,...` line on UART, laptop script forwards it to a Telegram bot | B (Norman) | tools/relay.sh (cat + grep + curl), main.c (print line, done in #6) | alerting, communication |
| E4 | Wi-Fi direct to laptop (replaces the USB cable in E3) | nobody yet | wifi_alert.c + the 4 files from the teacher's Wifi(optional).zip | communication |

E1-E3 are the target. E4 only if E1-E3 are merged and stable by 24 Sep; otherwise it is one
"future work" slide. Nothing else gets added after that.

## Contracts (write the .h first, then both sides can work in parallel)

### E1 fall_detector.h
```c
typedef enum {
    STATE_NORMAL, STATE_SUSPECT, STATE_CONFIRMING, STATE_FALL,
    STATE_LONG_LIE       /* FALL + LONG_LIE_MS with no ack, still lying */
} fall_state_t;
```
Button ack works in FALL and LONG_LIE. `fall_state_name()` returns "LONG_LIE".

### E2 buzzer.h  (Grove buzzer SIG on D6 = PB1 via the Grove base shield, GND, 3V3)
```c
typedef enum { BUZZER_OFF, BUZZER_SOS, BUZZER_SOS_FAST, BUZZER_CHIRP } buzzer_pattern_t;

void buzzer_init(void);                       /* PB1 as GPIO output, off */
void buzzer_set(buzzer_pattern_t p);          /* change pattern, restarts it */
void buzzer_update(unsigned int now_ms);      /* call every loop pass, never blocks */
```
Pattern = a small array of on/off durations in ms, played by comparing `now_ms` with the
time of the last edge, exactly like the LED blink in status_indicator.c. No HAL_Delay.
Morse SOS: dot 100, dash 300, gap 100, letter gap 300, word gap 1000. FAST = half of each.

### E3 UART alert line (printed by main.c the moment the state changes)
```
ALERT,FALL,<tick_ms>
ALERT,LONG_LIE,<tick_ms>
ALERT,ACK,<tick_ms>
```
The normal `state,acc,gyro,tilt,still` line every 500 ms stays as it is.
`tools/relay.sh` (bash: cat + grep + curl, no Python): opens the serial port, forwards every
`ALERT,` line to a Telegram chat via a bot created with @BotFather. Token and chat id are
read from `tools/secrets.sh`, which is git-ignored (commit `tools/secrets.sh.example`
instead). Message text:
- FALL: "FALL DETECTED. Wearer has not pressed OK."
- LONG_LIE: "NO RESPONSE FOR 30 s. Call an ambulance."
- ACK: "Alert acknowledged on the band."

## Rules that keep the core safe (these go on the integration slide)

1. The 20 ms sample -> filter -> state machine path does not change. Buzzer, alert prints
   and (later) Wi-Fi hang off the state *result*, never inside the decision.
2. Nothing in the loop blocks. Buzzer and LED are both tick-compared. UART prints stay
   short (one line).
3. Every enhancement is its own branch and PR, merged only after a board test whose serial
   log is pasted in the PR.
4. If the laptop script or Telegram is down, the board behaves exactly as before.

## Order and dates

| Date | A (Ruijie) | B (Norman) |
|---|---|---|
| 20-21 Sep | Review + merge PR #4. Run the 5 open test cases, screenshot for the slides. Start E2 buzzer (test with a 1 s beep first). | E1 Long Lie state + main.c alert lines. |
| 22-23 Sep | E3 relay.py + BotFather bot. Test with `ALERT,FALL,123` typed by hand before using the board. | Merge E1, help wire E2 into main.c, board test with the case on. |
| 24 Sep | Go / no-go on E4 Wi-Fi. | Threshold re-tune on the demo board, fall / near-fall trial table (10 trials each). |
| 25-26 Sep | Code freeze. Fresh-clone import test on the other laptop. | Same. |
| 27-29 Sep | Slides: EWMA + asm, sensor processing, test cases. | Slides: state machine, evidence, enhancements. Rehearsal together. |

## Evidence to collect as we go (needed for the slides, do not leave to the end)

- Serial log of a real fall and a near fall (have one from 20 Sep already, see PR #4).
- Screenshot of "ALL 5 OPEN TEST CASES PASSED".
- Trial table: 10 falls / 10 near falls / 10 normal activities, count of correct results.
- A phone screenshot of the Telegram message arriving.
- Photo of the board with the shield and buzzer in the case.
