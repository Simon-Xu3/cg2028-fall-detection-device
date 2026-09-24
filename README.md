# ElderCare Fall Detection Band

A wearable fall detector for elderly users, built on the STM32 B-L4S5I-IOT01A Discovery
board for NUS CG2028 (Computer Organization), September 2026.

The board reads its on-board accelerometer and gyroscope 50 times a second, low-pass
filters all six axes with an EWMA filter written in ARM assembly, and runs a five-state
machine that separates everyday movement, near falls and real falls. A confirmed fall
switches the LED to a fast blink and starts a Morse SOS on a buzzer until the wearer presses
the user button; if nobody presses it for 30 s the band escalates to `LONG_LIE`. Every state
change is visible on the serial port, and a small laptop script forwards the alerts to a
caregiver's Telegram.

## Features

- **EWMA filter in ARM Thumb-2 assembly** (`mov_avg.s`): integer-only,
  `y = (alpha * x + (100 - alpha) * y_old) / 100`, called from C through the AAPCS
  interface (`r0`-`r2` in, `r0` out, `r4`-`r7` preserved).
- **Assembly self-check**: every sample is also run through a C reference filter and
  compared axis by axis. A mismatch counter is printed if it ever becomes non-zero
  (it stayed at 0 in all board tests).
- **Sensor processing** (`sensor_processing.c`): six filtered axes are reduced to four
  features: acceleration magnitude (mg), gyroscope magnitude (dps), tilt angle (degrees
  from vertical) and a "still" flag.
- **Fall state machine** (`fall_detector.c`): `NORMAL -> SUSPECT -> CONFIRMING -> FALL -> LONG_LIE`.
  A fall needs an impact or free-fall spike, followed within 1 s by rotation, then a 2 s
  settle time after which the band must be tilted more than 60 degrees and still. Anything
  that does not complete the sequence returns to `NORMAL` (near fall). `FALL` latches until
  acknowledged; 30 s without acknowledgement escalates to `LONG_LIE`, and the detector
  records whether the wearer moved (300 ms debounce) so the alert can say so.
- **Status indicator** (`status_indicator.c`): LED2 slow blink (500 ms) normally, fast
  blink (100 ms) after a fall; user button with edge detection acknowledges the fall.
- **Buzzer** (`buzzer.c`): Grove buzzer on D6 plays Morse SOS in `FALL`, a faster SOS in
  `LONG_LIE`, one chirp on acknowledge. Pattern arrays played by tick comparison, no delays.
- **Caregiver alert** (`tools/relay.py`): the firmware prints one `ALERT,<event>,...` line
  the moment a fall is confirmed, escalates, or is acknowledged; the laptop script forwards
  each one to a Telegram group, including the home address and whether the wearer moved.
- **Non-blocking main loop** (`main.c`): 20 ms sampling and 500 ms serial reporting are
  both driven by `HAL_GetTick()` comparisons, so the LED and button stay responsive with
  no `HAL_Delay()` anywhere in the loop.
- **Serial CSV output** at 115200 baud, one line every 500 ms:
  `state,acc_mg,gyro_dps,tilt_deg,still`.

## Hardware

| Part | Use |
|---|---|
| B-L4S5I-IOT01A (STM32L4S5VI, Cortex-M4F @ 80 MHz) | main board |
| LSM6DSL accelerometer + gyroscope (on board, I²C) | motion sensing, accelerometer set to ±4 g at 52 Hz |
| LED2 and user button B2 (on board) | status and acknowledge |
| ST-LINK virtual COM port | serial output |
| Seeed Grove Buzzer v1.2 on a Grove base shield, socket D6 (PB1) | audible SOS (enhancement) |

No external parts are needed for the core detection; the buzzer is the only add-on.

## How it works

```
LSM6DSL ──accel x,y,z (mg)──┐
        ──gyro  x,y,z (dps)─┤ every 20 ms
                            ▼
              ewma_filter() in assembly, one call per axis
              (C reference runs in parallel, results compared)
                            ▼
              acc_mg, gyro_dps, tilt_deg, still
                            ▼
              fall_detector_update()  ──state──▶  status_indicator_update()  ──▶ LED2
                            │                                   ▲
                            └──────── every 500 ms ──▶ UART     └── user button ──▶ ack
```

State machine and thresholds (all `#define`s in `fall_detector.c`, tuned on the board):

| Transition | Condition |
|---|---|
| NORMAL -> SUSPECT | acc_mg < 500 (free fall) or acc_mg > 2000 (impact) |
| SUSPECT -> CONFIRMING | gyro_dps > 200 within 1 s |
| SUSPECT -> NORMAL | 1 s with no rotation (near fall) |
| CONFIRMING -> FALL | after a 2 s settle time: tilt_deg > 60 and still |
| CONFIRMING -> NORMAL | 5 s without lying still (near fall) |
| FALL -> LONG_LIE | 30 s without the user button |
| FALL or LONG_LIE -> NORMAL | user button |

## Board test (20 Sep 2026)

Real fall: board slammed onto a sofa, twisted, left on its side.

```
 4.3s  NORMAL,1059,0,3,1        flat, still
 5.3s  SUSPECT,1686,114,23,0    impact spike
 5.8s  CONFIRMING,571,53,97,0   rotation seen, board flipped
 8.3s  CONFIRMING,1119,41,86,0  on its side, not yet still
 8.8s  FALL,1008,12,89,1        still -> FALL, LED fast blink
```

Near fall: same impact and twist, then picked up flat again.

```
28.9s  NORMAL,881,9,15,1
29.4s  CONFIRMING,924,54,7,0
32.4s  NORMAL,1044,0,4,1        timeout, no alarm
```

The EWMA routine is also checked by a separate test project
(`CG2028_Assignment_Test`) that runs five open test cases with alpha = 25, 50, 10, 75, 33
and prints `ALL 5 OPEN TEST CASES PASSED`.

## Repository layout

```
code/
  CG2028_Assignment.zip_expanded/CG2028_Assignment/        main firmware (STM32CubeIDE project)
    Core/Src/main.c                                        20 ms loop, UART reporting
    Core/Src/mov_avg.s                                     EWMA filter in assembly
    Core/Src/sensor_processing.c  Core/Inc/sensor_processing.h
    Core/Src/fall_detector.c      Core/Inc/fall_detector.h
    Core/Src/status_indicator.c   Core/Inc/status_indicator.h
  CG2028_Assignment_Test.zip_expanded/CG2028_Assignment_Test/   EWMA test harness (same mov_avg.s)
Assignment/                                                assignment brief, test cases, rubric
ENHANCEMENT_PLAN.md                                        plan for the optional part
CG2028_A1_team_workflow.md                                 team workflow notes
```

## Build and run

1. Open STM32CubeIDE 1.19 and choose **File > Import > Existing Projects into Workspace**,
   pointing at `code/`. Import both projects.
2. Build `CG2028_Assignment` and flash it with **Run** (ST-LINK).
3. Open the ST-LINK serial port at 115200 baud, for example on macOS:

   ```
   screen /dev/cu.usbmodemXXXX 115200
   ```

4. You should see the header line followed by one CSV line every 500 ms. Flat on a desk
   it reads roughly `NORMAL,1050,0,2,1`.

To run the EWMA test cases, build and run `CG2028_Assignment_Test` the same way.

## Enhancements (done)

Planned in [ENHANCEMENT_PLAN.md](ENHANCEMENT_PLAN.md), all merged and tested on the board:

- `LONG_LIE` escalation 30 s after an unacknowledged fall, with a moved / not-moved flag.
- Grove buzzer: Morse SOS, faster SOS after escalation, chirp on acknowledge.
- Telegram alert to a caregiver group via `tools/relay.py` (`pip3 install pyserial requests`,
  copy `tools/secrets.py.example` to `tools/secrets.py`, run `python3 tools/relay.py`).

Test logs and phone screenshots are in [evidence/](evidence/README.md): real falls, the
30 s escalation with and without movement, all four Telegram messages, and the
false-positive / near-fall runs (walking, pocket, shaking, sitting down, slow lie-down)
with zero false alarms.

Known limits: no GPS (the home address is configured on the laptop), thresholds tuned on
two boards held in the hand rather than on a person, and the band is assumed to be worn
with its Z axis roughly vertical.

## Team

| | Name | Work |
|---|---|---|
| Member A | Xu Ruijie ([Simon-Xu3](https://github.com/Simon-Xu3)) | EWMA assembly, sensor processing, EWMA test cases |
| Member B | Lyu Jinhan ([normanqvq](https://github.com/normanqvq)) | fall state machine, status indicator, main loop, board testing, integration |

Both members review each other's pull requests; every feature goes through its own branch.
