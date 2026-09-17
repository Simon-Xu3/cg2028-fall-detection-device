# CG2028 Assignment 1 — ElderCare Wearable Safety Companion — FINAL PLAN (v2, 16 Sep 2026)

Board: B-L4S5I-IOT01A (STM32L4S5) in a 3D-printed case. IDE: STM32CubeIDE 1.19.
Only external part allowed: Grove Buzzer v1.2. Everything else is on the board.
Spec: ../assignment_requirements.md   Rubric: Marking_Rubric.pdf
Brainstorms: brainstorm_software.md, brainstorm_hardware.md, brainstorm_remote_alert.md

---------------------------------------------------------------------------------------------------
## 0. The product in one paragraph (this is also slide 1)

An elderly person wears the band. It samples the accelerometer and gyroscope 50 times a
second, cleans both with our assembly EWMA filter, and runs a state machine that only calls
"fall" when it sees free-fall or a fast rotation, THEN an impact, THEN a posture change,
THEN stillness. Sitting, bending, tilting and shaking never get past the first checks.
On a confirmed fall the band (1) beeps Morse SOS and lights the LED so people nearby come,
(2) sends a Telegram message to the caregiver's phone within seconds, and (3) if nobody
acknowledges — button on the band or reply on the phone — before the Long-Lie timer runs
out, escalates with a "no response, call ambulance 995" message that repeats until someone
answers. The caregiver can also mute, change sensitivity and read the fall report remotely.

---------------------------------------------------------------------------------------------------
## 1. Deadlines, people, submission

| What | When |
|---|---|
| Code + slides on Canvas, B01 (Wed AM) | 30 Sep 2026 08:00 |
| Code + slides on Canvas, B02 (Thu PM) | 1 Oct 2026 13:00 |
| Peer contribution declaration (Canvas > Quizzes > Assignment Peer Review) | 11 Oct 2026 12:00 |

We are in different lab sessions (Norman = Lab03, teammate = Lab02) but one group submits
once, so we work to the EARLIER date: **everything uploaded by Mon 29 Sep evening.**
Confirm with the teammate which of B01/B02 the sessions map to.
Group number = TBC. Matric1 = A0330854E (Norman). Matric2 = TBC.

Submit TWO separate files (do not zip them together):
- `Assignment1_<Group>_<Matric1>_<Matric2>_Code.zip`  — CubeIDE File > Export > Archive File
- `Assignment1_<Group>_<Matric1>_<Matric2>_Slides.pptx` (or pdf)
Pre-submit checklist:
- [ ] Re-import the zip into a CLEAN CubeIDE workspace, build, flash, run. Must take < 5 min (-2 if not)
- [ ] Board runs correctly with NO hotspot around (prints "WIFI: not joined", continues)
- [ ] Names + matric in the header comment of mov_avg.s
- [ ] `extern int ewma_filter(int, int, int)` signature unchanged
- [ ] No code change after the rehearsal

Work split (each person must still be able to explain everything in Q&A):
| Norman | Teammate |
|---|---|
| mov_avg.s + tests | Telegram bot + relay.py (laptop) |
| main.c: loop, features, state machine, button, console | wifi_alert.c (board Wi-Fi), buzzer pattern player |
| threshold trials + trial table | Wi-Fi/hotspot tests, demo network |
| slides 2-6 | slides 7-9 |

---------------------------------------------------------------------------------------------------
## 2. What the starter gives us and what we must fix in the core

Given:
- `Core/Src/mov_avg.s` — `ewma_filter` empty (PUSH/POP only). We write it.
- `Core/Src/main.c` — already inits HAL, UART1, LED2, accel, gyro; calls ewma_filter on all
  6 axes with separate old_output arrays; C reference `ewma_filter_C` + mismatch warning;
  prints a 320-byte status string; `HAL_Delay(1000)`. Only TODO: `int fall_detected = 0;`.
- `CG2028_Assignment_Test` project — 5 open tests (alpha 25/50/10/75/33), 16 samples each,
  asm vs C every sample via semihosting printf. 3 hidden tests: other alpha, non-zero start.
- Drivers in the zip: BSP for accel/gyro/magneto/humidity/pressure/temp/NFC, the
  `es_wifi.c` Wi-Fi module driver (compiled, unused), HAL SPI/QSPI.

Known problems to fix (core work, not enhancements — each one is a "problems and
solutions" slide bullet):
| # | Problem | Fix |
|---|---|---|
| 1 | `HAL_Delay(1000)` = 1 sample/s. A fall lasts < 1 s. | 20 ms loop timed with `HAL_GetTick()`; LED/buzzer are non-blocking |
| 2 | Accel full scale +-2 g (stm32l4s5i_iot01_accelero.c line 67). Our impact threshold ~2 g sits on the clipping ceiling. | after `BSP_ACCELERO_Init()` write CTRL1_XL (0x10) once = 52 Hz, +-4 g via `SENSOR_IO_Write(LSM6DSL_ACC_GYRO_I2C_ADDRESS_HIGH, 0x10, 0x48)`. The BSP re-reads the FS bits, so mg stays right. |
| 3 | CPU on 4 MHz MSI (no SystemClock_Config). BSP I2C timing 0x00702681 is designed for 80 MHz, so the sensor bus runs ~23 kHz and one accel+gyro read costs ~8-10 ms of the 20 ms budget. | copy `SystemClock_Config()` (MSI -> PLL 80 MHz) from Lab/workspace/WiFi_HTTP_Server/Src/main.c, call it right after `HAL_Init()`. Also required by the Wi-Fi SPI. Measure loop time before/after. |
| 4 | 320-byte status line = 28 ms at 115200, longer than one sample. | ~40-byte line every 10th loop; full report only on state change |
| 5 | UART is transmit-only (`HAL_UART_Transmit`). | add `HAL_UART_Receive_IT` 1 byte + `USART1_IRQHandler` for the console |

Units: accel raw = mg (1000 = 1 g). gyro raw / 1000 = deg/s. Sensor ODR 52 Hz, so a 20 ms
loop is matched; polling faster only returns duplicates.

---------------------------------------------------------------------------------------------------
## 3. Part 1 — mov_avg.s  (Norman, day 1, ~30 min, no hardware needed)

    out = (alpha * new + (100 - alpha) * old) / 100

    R0 = new_data, R1 = old_output, R2 = alpha_percent   (inputs)
    MUL  r3, r2, r0          ; r3 = alpha * new
    RSB  r4, r2, #100        ; r4 = 100 - alpha
    MLA  r3, r4, r1, r3      ; r3 = (100-alpha)*old + alpha*new
    MOV  r4, #100
    SDIV r0, r3, r4          ; truncates toward zero, same as C
Only R4 is callee-saved and used → `PUSH {r4, lr}` / `POP {r4, pc}`.
Overflow: |100 * 32767| ≈ 3.3 M fits int32. Stateless: old_output comes from the caller, so
hidden tests with non-zero initial output pass automatically.
Test: paste into the Test project, Debug, console must say "ALL 5 OPEN TEST CASES PASSED".
Then copy the same file into the app project. Never touch it again.

---------------------------------------------------------------------------------------------------
## 4. Part 2 — main.c core fall detection (Norman)

### 4.1 Loop skeleton (every 20 ms)
    read raw accel (int16 mg) + gyro (float mdps → int)
    6x ewma_filter (asm)            ← alpha per state (E2)
    6x ewma_filter_C                ← verification only, warn on mismatch
    compute features from asm values only
    read USER button (debounced, hold time)
    read one console byte if any
    run state machine (may change state; enter_state() sends alerts)
    update LED + buzzer patterns (HAL_GetTick based)
    every 10th loop: short UART status line
    if alert_pending and 5 s passed: retry the alert
    wait until 20 ms since last sample; count overruns

### 4.2 Features (from asm-filtered values ONLY, never raw)
| Feature | Formula | Unit | Rest value |
|---|---|---|---|
| acc_mag | sqrt(ax²+ay²+az²) | mg | ~1000 |
| gyro_mag | sqrt(gx²+gy²+gz²) / 1000 | deg/s | ~0-3 |
| tilt | acos(az / acc_mag) in degrees | deg | 0 when Z up |
| rest_tilt | tilt sampled while NORMAL and still | deg | — |
| still | \|acc_mag - 1000\| < 200 and gyro_mag < 30 for N samples | bool | — |

### 4.3 State machine (each state stores its entry tick)
    NORMAL       LED slow blink 1 s. → SUSPECT if
                   acc_mag < FREEFALL_TH   (vertical fall: free-fall dip)     OR
                   gyro_mag > GYRO_TH      (toppling fall: fast rotation)
    SUSPECT      500 ms window, track peak_acc / peak_gyro.
                   peak_acc > IMPACT_TH AND peak_gyro > GYRO_TH → CONFIRMING
                   timeout → NORMAL   (bump, shake, sat down hard)
    CONFIRMING   1000 ms window. LED fast blink, buzzer pre-alarm beeps ("press if OK").
                   |tilt - rest_tilt| > TILT_TH AND still → FALL
                   tilt unchanged AND moving at timeout → NORMAL
    FALL         LED solid, buzzer Morse SOS, alert #1 to caregiver.
                   button short press / console 'a' / phone ack → NORMAL
                   still for LONG_LIE_MS → LONG_LIE
                   (E3) upright + moving 3 s → RECOVERED → NORMAL
    LONG_LIE     faster SOS, alert #2 "call ambulance", repeated every 30 s.
                   Only an acknowledge leaves this state.
    (E3) CHECK_IN  from NORMAL after INACTIVE_MS without motion. Soft beep. Motion or
                   button → NORMAL; timeout → FALL-style alert.

Two fall types = two entry paths (draw both): vertical (free-fall dip then impact,
accel-driven) and toppling (little free-fall, gyro spike + posture change, gyro-driven).
Both must pass CONFIRMING. Say "angular velocity, deg/s" on the slide. One threshold on
one sensor scores 0 on the sensor-fusion row, hence the multi-stage confirmation.

Why near-falls are rejected:
- sit down: no free-fall, impact ~1200-1500 mg < IMPACT_TH, gyro slow → NORMAL/SUSPECT only.
- slow bend / tilt: no free-fall, no impact → never leaves NORMAL.
- shake / carry / walk: impact may be high but tilt unchanged and keeps moving → CONFIRMING
  times out.
- No flicker: windows + "still" requirement = confirmation, debounce, hysteresis (rubric words).

### 4.4 Threshold starting points (tune with the case on; units on the slide)
| Name | Start | Unit | Meaning |
|---|---|---|---|
| FREEFALL_TH | 600 | mg | acc_mag below this = free-fall dip |
| IMPACT_TH | 2000 | mg | peak after impact (needs +-4 g fix) |
| GYRO_TH | 250 | deg/s | fast rotation |
| TILT_TH | 45 | deg | posture change vs rest |
| STILL_ACC_BAND | 200 | mg | \|acc_mag-1000\| below this = no linear motion |
| STILL_GYRO | 30 | deg/s | below this = no rotation |
| SUSPECT_WINDOW | 500 | ms | |
| CONFIRM_WINDOW | 1000 | ms | |
| LONG_LIE_MS | 15000 | ms | demo value; real ~60 s |
| ALPHA_ACC / ALPHA_GYRO | 25 / 40 | % | quiet-state alpha (E2 raises it in SUSPECT/CONFIRMING) |
Sensitivity 1-3 (console + / -) scales IMPACT_TH and GYRO_TH by -25% / 0 / +25%.

### 4.5 Function layout in main.c (flat, one job each)
    read_sensors()        fills raw arrays
    filter_all()          6x asm + 6x C, mismatch check
    compute_features()    acc_mag, gyro_mag, tilt, still
    read_button()         returns BTN_NONE / BTN_SHORT / BTN_HOLD_2S
    read_console()        handles one received command letter, if any
    fall_state_machine()  switch(state) {...}, calls enter_state(new)
    enter_state(s)        sets state + entry tick, resets peaks, logs one line, calls send_alert()
    send_alert(kind)      UART SOS line always; wifi_alert_send() if USE_WIFI and joined
    update_led()          pattern by state
    update_buzzer()       pattern player: table of on/off ms per state, respects mute
    uart_status()         short line every 10 loops

---------------------------------------------------------------------------------------------------
## 5. Hardware: Grove buzzer + on-board button, LEDs, Wi-Fi

### 5.1 Buzzer — Seeed Grove Buzzer v1.2
Own transistor Q1 + R1, 3.3 V or 5 V, ~2300 Hz. Tone when SIG is HIGH → plain GPIO output.
| Buzzer pin | Wire | Board |
|---|---|---|
| GND | black | CN2 pin 6/7 GND |
| VCC | red | CN2 pin 4 3.3V (5 V if too quiet) |
| NC | white | not connected |
| SIG | yellow | D6 = PB1 |
Cable: if the far end is 4 loose female ends, plug straight on. If it is a Grove plug, take
4 male-to-male jumper wires from the lab (free supplies) and push them into the plug.
Tape the cable to the case. First test: touch SIG to 3.3 V → continuous tone = GPIO mode OK.

Patterns (non-blocking player, each pattern = small array of on/off ms):
| State | Pattern |
|---|---|
| CONFIRMING | 50 ms beep every 250 ms (pre-alarm) |
| FALL | Morse SOS: dot 100, dash 300, gap 100, letter gap 300, word gap 1000; loops |
| LONG_LIE | Morse SOS at double speed, no word gap |
| CHECK_IN | 100 ms beep every 2 s |
| acknowledge | one 100 ms chirp |
| manual SOS accepted | three 100 ms chirps |
| during a Wi-Fi send | forced ON (alarm never goes silent while the CPU is busy) |

### 5.2 On-board blue USER button B2 (PC13, pressed = LOW)
Read every loop, debounce = same reading for 2 loops (40 ms), measure hold time.
| Gesture | When | Action |
|---|---|---|
| short press (< 1 s) | CONFIRMING / FALL / LONG_LIE / CHECK_IN | "I'm OK" → NORMAL, cancels alerts |
| hold 2 s | NORMAL | manual SOS → FALL (three chirps first) |
The wearer cannot mute or change sensitivity by accident; the caregiver does that remotely.
Reset (black) is next to USER (blue) in the case: tape over the reset hole.

### 5.3 LEDs
| LED | Pin | Use |
|---|---|---|
| LD2 green | PB14 | required: slow blink NORMAL, fast blink CONFIRMING, solid FALL/LONG_LIE |
| LD1 green | PA5 | ON while muted; 1-3 blinks at boot = sensitivity; double blink = Wi-Fi joined |
LD1 shares PA5 with Arduino D13, unused. LD3/LD4 belong to Wi-Fi/BLE, leave them.

### 5.4 On-board Wi-Fi ISM43362 (SPI3 + PE1 data-ready, PE8 reset, PE0 CS)
Not an external part → allowed. Files:
- already in the zip: `Drivers/BSP/Components/es_wifi/es_wifi.c/.h`
- copy from `Lab/workspace/WiFi_HTTP_Server/Common/`: `es_wifi_io.c/.h`, `wifi.c/.h`,
  `es_wifi_conf.h` (ST's official example; reused, not student work)
- add to `stm32l4xx_it.c`: `EXTI1_IRQHandler`, `SPI3_IRQHandler`; `HAL_GPIO_EXTI_Callback`
  calls `SPI_WIFI_ISR()` for pin 1 (copy from the example)
- `SystemClock_Config()` 80 MHz (fix #3), because the SPI prescaler assumes 80 MHz
Pin conflicts: none with buzzer (PB1), button (PC13), LEDs (PB14/PA5), UART (PB6/PB7).

### 5.5 Pin summary
| Signal | Pin | Direction |
|---|---|---|
| Buzzer SIG | PB1 (D6) | output |
| USER button | PC13 | input, pull-up on board |
| LED2 / LED1 | PB14 / PA5 | output |
| UART1 to PC | PB6 / PB7 | starter |
| Wi-Fi | PC10/11/12 SPI3, PE0 CS, PE1 IRQ, PE8 reset, PB13 wake | example code |

### 5.6 Enclosure
Tune with the case on (it absorbs impact). Read the silkscreen through the slot for
"PWM/D6". Mounting direction is irrelevant (rest_tilt is learned). USB cable = power +
UART: keep slack during drops.

---------------------------------------------------------------------------------------------------
## 6. Enhancements — the alert chain (bell-curved, 0/1/3 per row)

Story for the slide: **Detect → Alert nearby → Alert caregiver → Escalate → Acknowledge.**

### 6.1 Layer A — local alert (teammate, 3-4 h)
Buzzer Morse SOS + LED per section 5. Button ack, manual SOS. Nothing in the decision path.

### 6.2 Layer B — caregiver phone alert via Telegram (the senior's A+ idea)
One Telegram bot, created with @BotFather on the teammate's phone. A caregiver chat and an
"emergency group" chat. Three delivery paths, all producing the same messages:

| Path | Board | Laptop | Status |
|---|---|---|---|
| G  UART gateway | prints `SOS,FALL,<tick>,<peak_acc>,<peak_gyro>,<sev>` on state entry; accepts `a` | `relay.py --serial`: forwards the line to Telegram; when the caregiver taps "I'm on it", writes `a` to the board | MUST, day 2 |
| W1 Wi-Fi → laptop relay, plain HTTP | joins the hotspot at boot; on state entry: TCP to laptop:8080, `POST /alert`, close (~300 ms) | `relay.py --http`: same forwarding; `GET /ack` returns 1 after the tap | TARGET, go/no-go 22 Sep |
| W2 Wi-Fi → Telegram direct over TLS | same as W1 but SSL socket to api.telegram.org:443 | none | STRETCH, 2-3 h spike after W1 |

Messages:
    FALL      "FALL DETECTED. Band #1. Impact 3.1 g, rotation 410 deg/s, severity HIGH.
               Wearer has not pressed OK. Tap 'I'm on it' or reply /ok."
    LONG_LIE  "NO RESPONSE FOR 15 s. CALL AMBULANCE 995 NOW."  (to caregiver + emergency
               group, repeated every 30 s until acknowledged)
    manual    "SOS BUTTON PRESSED by wearer."
    ack       "Alert acknowledged by <who> at <time>."   (from the band or from the phone)
Slide honesty line: "in deployment the escalation message would call an SMS/telco API;
here the escalation message IS the ambulance call."

Board code for W1/W2 = one new file `Core/Src/wifi_alert.c` (~150 lines, student style):
    int wifi_alert_init(void);            // WIFI_Init + WIFI_Connect(SSID, PASS); 1 = joined
    int wifi_alert_send(const char *msg); // open socket 0, send HTTP request, wait <= 2 s, close
    int wifi_alert_poll_ack(void);        // W1 only: GET /ack, 300 ms timeout, every 3 s in FALL/LONG_LIE
`#define USE_WIFI 1` and `#define ALERT_PATH ALERT_W1` in main.h. With USE_WIFI 0 the file
is not compiled and the board is exactly the UART version.
Rules that keep the core safe (say them on the integration slide):
1. Wi-Fi is touched only at boot (before the loop) and inside `send_alert()` on state entry.
2. Every Wi-Fi call has a timeout ≤ 2 s. A failed send sets `alert_pending`; the main loop
   retries every 5 s. No loops waiting for the module.
3. Buzzer is forced ON during a send; LED state is untouched.
4. If the hotspot is missing at boot, print "WIFI: not joined, UART alerts only" after ≤ 10 s
   and run normally. The GA's import test therefore works without our hotspot.
5. The overrun counter shows exactly one long loop per alert and zero otherwise.

W2 extra: es_wifi.c `ES_WIFI_StartClientConnection` only sends the remote IP (P3) when the
type is plain TCP; add `|| conn->Type == ES_WIFI_TCP_SSL_CONNECTION` to that `if`. TLS on
this module is unverified by us: the spike is `GET /bot<token>/getMe` — if it does not
return HTTP 200 in one evening, drop W2. W2 also puts the bot token on the board; W1 keeps
it on the laptop, which is the better security story anyway.

### 6.3 Layer C — software depth (Norman, after the core works)
| Step | Feature | Hours | Rubric words |
|---|---|---|---|
| E1 | Loop-overrun counter + boot self-test line (sensor WHO_AM_I, Wi-Fi joined?, chirp) | 1-2 | integration evidence |
| E2 | Adaptive alpha: 15-25 in NORMAL/FALL, 60 in SUSPECT/CONFIRMING, same asm routine | 2-3 | adaptive filtering |
| E3 | RECOVERED + CHECK_IN states | 4-5 | state-machine extension, inactivity monitoring, Long Lie escalation |
| E4 | Ring buffer of last 2 s + fall report (peak g, peak deg/s, impact axis, time-to-still, severity) printed one line per loop and summarised in the Telegram message | 3-4 | logging, communication |
| E5 | Caregiver console over UART/relay: a ack, s status, + / - sensitivity, m mute, t self-test, d dump | 3-4 | communication, remote acknowledge |
| E6 | QSPI black box (persist E4 across power cycle) | 5-8 | persistent logging — only if everything above is frozen by 24 Sep |
Optional after that, one at a time: barometer height-drop advisory (sensor fusion word),
double-tap-to-acknowledge in FALL only. Never add anything in the demo week.

### 6.4 Rubric mapping (quote these nouns on the slides)
| Rubric row (3 marks) | 3-mark wording | Our evidence |
|---|---|---|
| Relevance to elderly user | "Long Lie escalation, SOS, inactivity monitoring, alerting, or logging" | Morse SOS, Telegram SOS, ambulance escalation, CHECK_IN, fall report |
| Technical depth | "state-machine extension, adaptive filtering, sensor fusion, communication, logging, or a meaningful algorithmic change" | E3, E2, two-path entry, Wi-Fi/UART communication, E4 |
| Integration & robustness | "does not degrade compulsory fall detection and remains stable throughout the demonstration" | overrun counter 0, timeouts, hotspot-less boot, trial table |

---------------------------------------------------------------------------------------------------
## 7. Schedule (16 → 29 Sep, deadline 30 Sep 08:00)

| Date | Norman | Teammate |
|---|---|---|
| Tue 16 | import both projects; mov_avg.s passes 5 open tests; copy to app | create bot with @BotFather; relay.py --serial forwarding a typed test line to the phone; take jumper wires |
| Wed 17 - Thu 18 | 80 MHz clock, +-4 g fix, 20 ms loop, non-blocking LED, short status line; log acc_mag/gyro_mag/tilt for drop, sit, bend, tilt, shake, carry | Wi-Fi spike in a COPY of Lab/workspace/WiFi_HTTP_Server: join hotspot, POST to relay.py → message on phone (W1) |
| Fri 19 - Sun 21 | state machine + button + console (E5) | buzzer wiring + pattern player + Morse SOS; wifi_alert.c merged into the app under USE_WIFI |
| Mon 22 | **go/no-go W1 and W2.** E1 | W2 TLS spike (getMe) only if W1 is done |
| Tue 23 - Wed 24 | E2, E3, E4 | ack-from-phone, escalation repeat, hotspot-less boot test |
| Thu 25 - Sat 27 | trial table with the case on (section 8), tune, re-run | Wi-Fi robustness tests, demo network in the real lab room |
| Sun 28 | slides 2-6, plots from UART logs | slides 1, 7-9; demo script |
| Mon 29 | rehearsal x2 with hotspot; clean-workspace import test; export; **submit** | same |
| Tue 30 | nothing. Deadline 08:00. | |

---------------------------------------------------------------------------------------------------
## 8. Testing and evidence (becomes the evidence slide)

5 trials each, case on, USB slack. Record peak acc_mag, peak gyro_mag, tilt change, state.
| Motion | Expected |
|---|---|
| Cushion drop, flat | FALL + Telegram within 3 s |
| Cushion drop, tipping over | FALL |
| Drop from 10 cm vs 40 cm | FALL both, severity differs (E4) |
| Light shaking | NORMAL |
| Casual tilting | NORMAL |
| Carrying / walking-like swing | NORMAL |
| Medium-speed bending | NORMAL |
| Slow-controlled lowering (sit down) | NORMAL |
| Sit down hard | NORMAL (SUSPECT at most) |
| Fall, pick up and walk (E3) | RECOVERED |
| Fall and leave 15 s | LONG_LIE + ambulance message, repeats every 30 s |
| Button during LONG_LIE / tap on phone | NORMAL + "acknowledged" message |
| Boot with hotspot off | "WIFI: not joined", detection still works |
| Hotspot dropped mid-run, then a fall | UART SOS still printed, Wi-Fi retry every 5 s |
Also: overrun counter stays 0 during buzzer + log dump; one long loop per alert. Keep raw
UART logs of one clean fall and one sit-down for the plot slide.

---------------------------------------------------------------------------------------------------
## 9. Slides and 10-minute demo

Slides: 1 product story (section 0) · 2 architecture: sensors → asm EWMA (6 states) →
features → state machine → LED/buzzer/Telegram · 3 EWMA asm: equation, registers, test
screenshot · 4 sensor processing: units, alpha plot, +-4 g, 80 MHz/I2C · 5 state diagram
with both entry paths + threshold table · 6 evidence: trial table, fall-vs-sit plot ·
7 alert chain: buzzer, Telegram, escalation, ack (screenshots of the phone) · 8 software
depth E1-E4 + integration rules · 9 problems/solutions + limitations + future (telco API,
BLE) · 10 appendix: contributions.

Demo script:
1. Power on: self-test line, LD1 double blink = Wi-Fi joined, phone shows "band online". [1 min]
2. Tilt, shake, carry → NORMAL. Sit slow, bend → NORMAL. Sit hard → SUSPECT → NORMAL. [2 min]
3. Cushion drop → CONFIRMING beeps → FALL: LED solid, Morse SOS, phone buzzes with the
   fall summary. [1 min]
4. Wait 15 s → LONG_LIE, "CALL AMBULANCE" message. Tap "I'm on it" on the phone → band
   chirps, NORMAL. [1.5 min]
5. Drop again, pick up and walk → RECOVERED (E3). Hold button 2 s → manual SOS. [1 min]
6. Console: `d` fall report scroll (E4), `+` sensitivity, `m` mute (LD1 on). [1 min]
7. Q&A buffer. [2.5 min]
Backup if the hotspot misbehaves: unplug nothing, the UART gateway path shows the same
messages. Both students explain the asm and the state machine.

---------------------------------------------------------------------------------------------------
## 10. Rubric summary

| Part | Marks |
|---|---|
| Fall detection: TP / FP / near-fall | 3 x 3 = 9 |
| Architecture / sensor fusion / responsiveness | 3 x 2 = 6 |
| Enhancement: relevance / depth / integration | 3 x 3 = 9, each 0/1/3, bell-curved |
| Presentation per student + Q&A | 4 + 2 each |
| EWMA tests 8 x 2 | 16 |
| Penalties | import > 5 min -2; each glitch/restart -1 (max -3); code quality -1; C helper or clobbered R4-R11 in asm -2 |
Highest value per hour: threshold tuning (9 marks) and a glitch-free demo (up to -3).

---------------------------------------------------------------------------------------------------
## 11. Repo workflow
- Commits and pushes under Norman's own git identity. No Claude attribution, no Claude collaborator.
- `.gitignore`: `Debug/`, `.DS_Store`, `Core/Inc/secrets.h`. KEEP `.cproject`, `.project`,
  `.settings/`, `.mxproject`, the `.ld` linker script so the GA's import works.
- `secrets_template.h` is committed (SSID, password, bot token placeholders). The Canvas zip
  contains the real `secrets.h`.
- Commit after each schedule row with a message saying what works.
- The Canvas zip is exported from CubeIDE, not from git, so Debug/ is included as expected.

---------------------------------------------------------------------------------------------------
## 12. Open questions
- [ ] Group number; teammate's matric; which of Lab02/Lab03 is B01/B02
- [ ] Far end of the buzzer cable: loose female ends or Grove plug?
- [ ] Which case slot exposes CN3 (D0-D7); tape over reset
- [ ] Whose phone is the hotspot; confirm 2.4 GHz works in the lab room
- [ ] Bot created? caregiver chat id + emergency group chat id noted in relay.py
- [x] Jumper wires: free lab supplies
- [x] Hardware limit: Grove buzzer only; on-board Wi-Fi allowed
- [x] Deadline: work to 30 Sep 08:00

---------------------------------------------------------------------------------------------------
## 13. Code style rule (lecturer: must understand every AI line or no marks)
- Flat functions, a few globals. No function pointers, no macro tricks, no clever bit hacks.
- One `#define` per threshold, unit in the name (`IMPACT_TH_MG`, `SUSPECT_WINDOW_MS`).
- Plain `switch(state)` / `if-else`. Comments say WHY, one per asm line.
- Wi-Fi code calls the ST `WIFI_*` functions only; we do not modify the driver except the
  one documented `if` for W2.
- Before Q&A each of us reads every file once more and can answer "what does this line do".
