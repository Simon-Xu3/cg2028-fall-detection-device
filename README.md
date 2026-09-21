# ElderCare Fall Detection Band

A wearable fall detector for elderly users, built on the STM32 B-L4S5I-IOT01A Discovery
board for NUS CG2028 (Computer Organization), September 2026.

The board reads its on-board accelerometer and gyroscope 50 times a second, low-pass
filters all six axes with an EWMA filter written in ARM assembly, and runs a four-state
machine that separates everyday movement, near falls and real falls. A confirmed fall
switches the LED to a fast blink until the wearer presses the user button, and every state
change is visible on the serial port.

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
- **Fall state machine** (`fall_detector.c`): `NORMAL -> SUSPECT -> CONFIRMING -> FALL`.
  A fall needs an impact or free-fall spike, followed within 1 s by rotation, followed by
  lying still for 2 s. Anything that does not complete the sequence returns to `NORMAL`
  (near fall). `FALL` latches until acknowledged.
- **Status indicator** (`status_indicator.c`): LED2 slow blink (500 ms) normally, fast
  blink (100 ms) after a fall; user button with edge detection acknowledges the fall.
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

No external parts are needed for the core system.

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
| CONFIRMING -> FALL | after 2 s: tilt_deg > 60 and still |
| CONFIRMING -> NORMAL | 5 s without lying still (near fall) |
| FALL -> NORMAL | user button |

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

## Planned enhancements

See [ENHANCEMENT_PLAN.md](ENHANCEMENT_PLAN.md). In order: a `LONG_LIE` state that
escalates when a fall is not acknowledged within 30 s, a Grove buzzer playing Morse SOS,
and a Telegram alert to a caregiver's phone relayed from the serial port.

## Team

| | Name | Work |
|---|---|---|
| Member A | Xu Ruijie ([Simon-Xu3](https://github.com/Simon-Xu3)) | EWMA assembly, sensor processing, EWMA test cases |
| Member B | Lyu Jinhan ([normanqvq](https://github.com/normanqvq)) | fall state machine, status indicator, main loop, board testing, integration |

Both members review each other's pull requests; every feature goes through its own branch.
