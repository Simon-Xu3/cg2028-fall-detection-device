# Slides outline and demo script

For the CG2028 Assignment 1 presentation (deadline 30 Sep 2026, 08:00). Everything the
rubric asks for is mapped to a slide below, with the exact content, the diagram each slide
needs, who presents it, and the evidence file to pull from. Build the deck in this order:
the rubric's own "slide structure" row lists it as *user problem, requirements, filter
design, implementation, detection logic, evidence, enhancements, conclusions*.

Presenter split: **A = Xu Ruijie** (EWMA assembly, sensor processing, test cases),
**B = Lyu Jinhan** (state machine, indicator, main loop, board tests, enhancements).
Both must be able to answer questions on every slide; Q&A is 2 marks each and is random.

---

## 1. Rubric map (what each mark needs, where it lives)

| Rubric row | Marks | What the 3-mark / 1-mark wording asks for | Slide | Evidence |
|---|---|---|---|---|
| Fall detection reliability (TP) | 3 | reliable across repeated trials and small variations in height, angle, motion | 9, demo | `evidence/2026-09-20_real_fall.csv`, 23 Sep LONG_LIE logs, trial table |
| False alarms during normal use (FP) | 3 | near-zero false alarms during carrying, walking-like motion, tilting, light shaking | 9, demo | `evidence/2026-09-24_false_positive_*.csv` (0 false alarms) |
| Near-fall discrimination | 3 | sitting down, medium-speed bending, slow controlled lowering are not falls | 8, 9, demo | `evidence/2026-09-24_near_fall_sit_bend_lie_down.csv` (0 false alarms, tilt reached 130) |
| Architecture and data flow | 2 | sensing -> assembly EWMA -> features -> state logic -> outputs, matches the code, EWMA on all axes with separate recursive states | 3 | `sensor_processing.c` (`accel_asm_old[3]`, `gyro_asm_old[3]`) |
| Sensor fusion, features, thresholds | 2 | both sensors, explainable features, thresholds in physical units | 7, 8 | `fall_detector.c` defines |
| Responsiveness, stability, alert behaviour | 2 | prompt stable transitions; confirmation, debounce, hysteresis | 8, demo | 3-step confirmation, 300 ms movement debounce, FALL latch |
| Enhancement: relevance | 3 | Long Lie escalation, SOS, alerting | 10 | LONG_LIE, buzzer, Telegram |
| Enhancement: technical depth | 3 | state-machine extension, communication | 10 | fifth state + moved flag, UART ALERT protocol + relay |
| Enhancement: integration and robustness | 3 | does not degrade core detection, stable through the demo | 10, demo | buzzer/relay hang off the state result; relay timeout; core untouched |
| Slides: content accuracy | 1 | assembly interface, recursive state, sensor units, state logic, results, limitations | 4, 5, 7, 8, 9, 11 | |
| Slides: visual design | 1 | not text-heavy, consistent | all | one diagram or table per slide, max 4 bullets |
| Slides: diagrams and evidence | 1 | labelled pipeline and state diagrams, test evidence | 3, 8, 9 | |
| Slides: structure and flow | 1 | the order above | all | |
| Presentation per student (4 each) | 8 | content, structure, delivery, demo integrated with the explanation | | rehearse twice |
| Q&A per student | 4 | | section 5 | |
| EWMA test cases | 16 | 5 open cases + hidden ones, run in `CG2028_Assignment_Test` | 6 | screenshot "ALL 5 OPEN TEST CASES PASSED" |
| Penalties | -2 / -3 / -1 / -2 | import > 5 min; glitch or restart; code style; C helper or clobbered r4-r11 | | fresh-clone build verified; use a good USB cable |

---

## 2. The pipeline (slide 3 diagram, and the story of the whole deck)

```
LSM6DSL accelerometer ──ax,ay,az (mg)──┐
LSM6DSL gyroscope     ──gx,gy,gz (dps)─┤  read every 20 ms (sensor runs at 52 Hz)
                                       ▼
        ewma_filter() in ARM assembly, called once per axis (6 calls, 6 separate old_output states)
        alpha = 25 %  ->  y = (25·x + 75·y_old) / 100
        (a C copy runs beside it; results compared axis by axis; mismatch counter stayed 0)
                                       ▼
        four features:  acc_mg = |a|   gyro_dps = |g|   tilt_deg = angle(Z, vertical)   still = |a|≈1 g and |g|≤15
                                       ▼
        fall state machine, 3 gates in sequence:  impact/free-fall  ->  rotation within 1 s  ->  tilted and still
        NORMAL -> SUSPECT -> CONFIRMING -> FALL -> LONG_LIE
                                       ▼
        outputs every loop pass, none of them blocking:
          LED2   slow blink 500 ms / fast blink 100 ms
          buzzer Morse SOS / fast SOS / chirp on acknowledge
          UART   one CSV line every 500 ms + one ALERT line on each state change
                                       ▼
        laptop: tools/relay.py reads the ALERT lines and posts to the caregiver's Telegram group
```

One sentence for the audience: *the band turns six raw axes into four physical features,
and a fall is only declared when impact, rotation and a change of posture happen in that
order within a few seconds; everything downstream (LED, buzzer, phone) hangs off that
decision and never feeds back into it.*

---

## 3. Slide-by-slide

### Slide 1 — Title and the user problem (A)
- Title: *ElderCare Band: fall detection with Long Lie escalation on STM32.* Team names, matric numbers.
- One line of context: one in three people over 65 fall each year; the danger is the *long lie*, lying on the floor for a long time because nobody knows. (Ask the medical-school friend for the source, or cite a WHO fall fact sheet.)
- What the band does in three verbs: **detect** the fall, **alert** people nearby and a caregiver, **escalate** if nobody responds.

### Slide 2 — Requirements (A)
- Compulsory: EWMA low-pass filter in ARM assembly on both accelerometer and gyroscope; distinguish normal activity, near falls and real falls; LED slow blink normally, fast blink after a fall; serial output.
- Our additions: LONG_LIE escalation, buzzer SOS, caregiver alert.
- Hardware: B-L4S5I-IOT01A (STM32L4S5, 80 MHz), on-board LSM6DSL, LED2, user button, one Grove buzzer. Photo of the board in the case.

### Slide 3 — Architecture and data flow (A)
- The pipeline diagram from section 2, drawn as boxes and arrows with the file name under each box (`mov_avg.s`, `sensor_processing.c`, `fall_detector.c`, `status_indicator.c`, `buzzer.c`, `main.c`, `tools/relay.py`).
- Say out loud: six axes, six calls, six separate recursive states; only the assembly output feeds the detector; the C copy is verification only.

### Slide 4 — EWMA filter design (A)
- Equation: `y[n] = (alpha · x[n] + (100 − alpha) · y[n−1]) / 100`, integer percent alpha, signed, truncation toward zero.
- Why alpha = 25: a step settles in about 4 samples (80 ms), fast enough for an impact, slow enough to kill sensor jitter. Show a small plot: raw vs filtered for one axis from a real fall log.
- Recursive state: `y[n−1]` per axis, reset to 0 at boot.

### Slide 5 — Assembly interface (A)
- Prototype: `int ewma_filter(int new_data, int old_output, int alpha_percent);`
- AAPCS table: r0 new_data (and return), r1 old_output, r2 alpha, r4–r7 callee-saved and preserved with PUSH/POP, lr/pc.
- The 8 instructions with one comment each (MOVS, MUL, SUB, MUL, ADD, SDIV, POP). Say "SDIV truncates toward zero exactly like C, which is why the C comparison matches bit for bit".
- Compliance line: no C helper, no floating point, r4–r7 saved.

### Slide 6 — EWMA test evidence (A)
- Screenshot of `CG2028_Assignment_Test` printing `ALL 5 OPEN TEST CASES PASSED` (alpha 25, 50, 10, 75, 33; negative inputs and truncation covered by cases 2 and 3).
- Runtime self-check: every sample also runs the C reference, per-axis comparison, `mismatch_count` printed if non-zero; it stayed 0 through every test. One CSV line without a WARNING as proof.

### Slide 7 — Sensor processing: six axes to four features (B)
- Table: acc_mg (√ of the three filtered axes, mg, ≈1000 at rest), gyro_dps (same for gyro), tilt_deg (atan2 of the horizontal component and Z, 0 flat, 90 on its side), still (acc within 1000 ± 120 and gyro ≤ 15).
- Three settings and why: accelerometer ±4 g (`CTRL1_XL = 0x38`) because impacts reach 2–3 g and ±2 g clips; 80 MHz system clock because the BSP I²C timing assumes it; 20 ms sampling because the sensor runs at 52 Hz.
- Why magnitude: orientation-independent, so it works however the band is worn.

### Slide 8 — Fall state machine (B)
- State diagram, five states, every arrow labelled with its condition and time window:

```
NORMAL ──acc<500 or acc>2000──▶ SUSPECT ──gyro>200 within 1 s──▶ CONFIRMING
  ▲                                │                                  │
  │◀──── 1 s, no rotation ─────────┘                                  │
  │◀──── 5 s, never lying still ──────────────────────────────────────┤
  │                                   ≥2 s settle, then tilt>60 & still│
  │◀── button ── FALL ◀─────────────────────────────────────────────────┘
  │                │ 30 s, no button
  │◀── button ── LONG_LIE
```

- Threshold table with units and reasons:

| Define | Value | Reason |
|---|---|---|
| FREEFALL_MG | 500 | free fall reads well below 1 g |
| IMPACT_MG | 2000 | landing spike above 2 g; hard sitting stayed below it in tests |
| ROTATION_DPS | 200 | the body turns during a fall; sitting/bending do not reach it after an impact |
| LYING_TILT_DEG | 60 | more than 60° from upright means lying |
| SUSPECT_TIMEOUT_MS | 1000 | rotation must follow the impact quickly |
| CONFIRM_STILL_MS | 2000 | let the body settle before judging |
| CONFIRM_TIMEOUT_MS | 5000 | give up if the person is back up (was 3000, raised after a board test reached FALL at 3.1 s) |
| LONG_LIE_MS | 30000 | demo-length escalation; literature uses much longer |
| MOVED_MS | 300 | 15 consecutive samples of movement before "moved" is set, so cushion settling does not count |

- Say the three rubric words: **confirmation** (three gates in sequence), **debounce** (300 ms movement, edge-detected button), **hysteresis / latch** (FALL only leaves on the button).
- Why three gates and not one threshold: a hard sit passes gate 1 only; a turn passes gate 2 only; lying in bed passes gate 3 only. Only a fall passes all three in order.

### Slide 9 — Board test evidence (B)
- Real fall log (left), near fall log (right), from `evidence/README.md`; annotate the rows where each gate opens.
- Trial table (fill in on the demo board with the case on):

| Scenario | Trials | Correct | Log |
|---|---|---|---|
| Real fall, chest height onto sofa | | | |
| Real fall, waist height / other angle | | | |
| Walking with the band in hand / in a pocket | 6 actions | 6/6 no alarm | `2026-09-24_false_positive_*.csv` |
| Sitting down hard, bending, slow lie-down, squat | 5 actions | 5/5 no alarm | `2026-09-24_near_fall_sit_bend_lie_down.csv` |
| LONG_LIE after 30 s, untouched / nudged | 2 | 2/2 | `2026-09-23_longlie_*.csv` |

- One line: "slow lie-down reached tilt 130° and still stayed NORMAL, because there was no impact spike: gate 1 never opened."

### Slide 10 — Enhancements (B)
- Story arrow: **detect → alert nearby → alert caregiver → escalate → acknowledge**.
- LONG_LIE: fifth state, 30 s without the button, plus the moved flag (300 ms debounce) so the caregiver knows if the person is responsive. This came from asking a medical student what responders need: location and whether the person is conscious.
- Buzzer: Morse SOS (dot 100, dash 300, letter gap 300, word gap 1000), fast SOS after escalation, chirp on acknowledge. Pattern arrays played by tick comparison, no delays; hangs off the state result.
- Telegram: firmware prints `ALERT,FALL,<tick>,moved=0` etc.; `relay.py` (pyserial + requests, 30 lines) posts to a group; messages include the configured home address and "Call 995 now" for the no-movement case. Phone screenshot `evidence/2026-09-23_telegram_all_four_messages.jpg`.
- Integration line: the 20 ms sample → filter → state path is untouched; all three enhancements read the state, never write it; the board behaves identically if the laptop script is off.

### Slide 11 — Limitations and conclusion (both)
- Limitations (say them before they are asked): the band is assumed worn flat (Z up), so a fall ending face-up or activity with the band held sideways is outside the tested range (future work: compare orientation before and after the impact instead of absolute tilt); falls that end in a sitting posture are not distinguished from sitting down hard (needs a height estimate, e.g. the on-board barometer); no GPS, the address is configured; thresholds tuned on two boards in the hand, not on a person; Telegram needs the laptop relay (future work: on-board Wi-Fi to the same bot).
- Conclusion: three-gate detector with zero false alarms in the daily-activity and near-fall runs, verified assembly EWMA, and an alert chain that reaches a phone in under two seconds.
- Backup slide: contributions table (from README) and the full threshold list.

---

## 4. Demo script (4 minutes, integrated into slides 8–10)

Set-up before starting: board flashed from `main`, good USB cable strapped to the table, laptop on a phone hotspot (campus Wi-Fi may block api.telegram.org), `python3 tools/relay.py` running and showing `NORMAL,...` lines, phone with the Telegram group open and visible, buzzer plugged into D6, shield switch on 3V3, black RESET button taped over.

| Step | Do | Say | Rubric row |
|---|---|---|---|
| 1 | Board flat on the table, show the serial window | "NORMAL, about 1000 mg, tilt 2, still 1, and no mismatch warning: assembly and C agree on every sample." | architecture, EWMA |
| 2 | Pick it up flat, walk five steps, shake lightly, tilt it casually, put it down | "Everyday movement: it may flash through SUSPECT but never alarms." | FP |
| 3 | Sit down hard on the chair holding it flat; then lie it down slowly on the cushion | "Sitting: impact but no rotation, back to NORMAL in a second. Slow lie-down: tilt 90 but no impact, still NORMAL." | near-fall |
| 4 | Real fall: chest height, slam onto the cushion with a wrist twist, leave it on its side | "Impact, rotation, then lying still: FALL. LED fast, SOS, and the phone." Show the Telegram message. | TP, responsiveness, enhancement |
| 5 | Wait 30 s without touching it (talk through slide 10 meanwhile) | "Nobody responded: LONG_LIE, faster SOS, and the caregiver gets 'Call 995 now' with the address." | enhancement |
| 6 | Press the blue button | "One chirp, back to NORMAL, phone says acknowledged." | alert behaviour |
| 7 | Second fall from a different height or angle | "Same result: repeatable." | TP variations |

If the hotspot fails: the board still does everything; say "the alert line is on the serial window, the relay just could not reach Telegram" and move on. If the USB cable drops, the board restarts: that is a -1, so the cable is strapped.

---

## 5. Q&A preparation (both students)

**Filter and assembly**
- Why EWMA and not a moving average? One multiply-add per sample, no buffer, and one parameter to tune.
- Why alpha 25? Settles in ~4 samples (80 ms): fast enough to keep an impact spike, slow enough to remove jitter. 10 would smear the impact, 75 would barely filter.
- Which registers are saved and why? r4–r7 with PUSH/POP because AAPCS says the callee must preserve r4–r11; r0–r3 are scratch; the result is returned in r0.
- Why SDIV? Signed inputs (negative axes) and truncation toward zero, identical to C, which is what makes the runtime comparison valid.
- How do you know the assembly is right? Five open test cases including negative and alternating inputs, plus the runtime comparison against the C reference on every sample: the mismatch counter stayed at 0.

**Features and thresholds**
- Why magnitudes? Independent of how the band is oriented on the body.
- Why tilt as well as still? Bending to tie a shoe tilts the band but the person is moving; lying on the floor is tilted *and* still.
- Where do the numbers come from? Physical reasoning first (free fall < 0.5 g, impact > 2 g, lying > 60°), then tuned on the board; the confirm timeout was raised from 3 to 5 s after a logged fall reached FALL at 3.1 s.
- Why 20 ms? The sensor updates at 52 Hz; faster is duplicate reads, slower can miss the impact peak.
- Why ±4 g? Impacts of 2–3 g clip at ±2 g and the 2000 mg gate would never open.

**State machine**
- Why three gates? Each one alone is passed by a normal action (sit, turn, lie down); only a fall passes all three in order and inside the time windows.
- What stops flicker? FALL latches until the button; movement after the fall needs 300 ms in a row to count; the button is edge-detected.
- Why does FALL not clear itself? The band cannot know the person is fine; a missed fall costs far more than one button press.
- What is the main loop doing? A single non-blocking loop keyed on HAL_GetTick(): 20 ms sampling, LED and buzzer updates every pass, 500 ms serial line. No HAL_Delay anywhere, so nothing starves the button.

**Enhancements**
- Why 30 s? Demo length; the define is one number. Real long-lie definitions are an hour or more.
- What does moved mean? Whether the person moved for at least 300 ms continuously after the fall; no button and no movement is the most urgent message.
- Who calls the ambulance? The caregiver who receives the message; the device never dials emergency services itself because a false alarm would waste an ambulance.
- Why not send from the board over Wi-Fi? A blocking network call inside the 20 ms loop would stall detection, and the campus network is unreliable; the laptop relay keeps the firmware clean. On-board Wi-Fi to the same bot is the natural next step.
- What if the laptop is off? The board still detects, blinks and sounds the SOS; only the phone message is missing.

**Limitations**
- Sideways-held band, face-up landing, sitting-type falls, no GPS: see slide 11, answer them as design scope, not as bugs.

---

## 6. Files to pull from

- Code: `code/CG2028_Assignment.zip_expanded/CG2028_Assignment/Core/Src/{mov_avg.s, sensor_processing.c, fall_detector.c, status_indicator.c, buzzer.c, main.c}`, `tools/relay.py`.
- Evidence: `evidence/README.md` lists every log and screenshot with what it shows.
- Plan and history: `ENHANCEMENT_PLAN.md`, `README.md`, the pull requests #2–#16.
