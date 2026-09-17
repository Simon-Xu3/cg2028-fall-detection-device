# Software-only enhancement candidates (brainstorm result)

Framing: buzzer + switch already cover RELEVANCE. What they lack is TECHNICAL DEPTH.
Most groups will show up with a buzzer and a button, so on a bell curve depth is where
software separates us. INTEGRATION is 3-or-nothing: one blocking UART dump or flash erase
in the sample loop = missed fall in front of the GA.

| # | Idea | Effort | Rubric words hit | Risk |
|---|---|---|---|---|
| 1 | Black-box ring buffer (last 2 s) + fall report over UART | 3-4 h | logging, communication | dump one line per loop, never one big transmit |
| 2 | Adaptive alpha: quiet alpha in NORMAL (15), sharp alpha in SUSPECT/CONFIRMING (60), passed to the same asm filter | 2-3 h | adaptive filtering | zero timing cost; re-tune thresholds after enabling |
| 3 | Start-up self-calibration: gyro bias, noise floor, derive thresholds, reject if moved | 3-4 h | adaptive thresholds, engineering justification | clamp derived thresholds |
| 4 | Complementary-filter tilt: 0.98*(prev + gyro*dt) + 0.02*accel_tilt | 6-8 h | SENSOR FUSION (strongest word) | sign/axis bugs → keep accel-only posture as #define fallback |
| 5 | Auto-recovery: upright + moving 3 s in FALL → RECOVERED → NORMAL. LONG_LIE never auto-cancels | 2-3 h | state-machine extension, recovery acknowledgement | upright condition prevents thrashing cancel |
| 6 | Inactivity wellness check-in NORMAL: no motion N min → CHECK_IN beep → SOS | 2-3 h | inactivity monitoring (named), state extension | separate thresholds from fall detection |
| 7 | Fall severity grading (peak acc, peak gyro, time-to-still) + direction; SEVERE shortens long-lie timer | 3 h | algorithmic change, logging, escalation | none |
| 8 | UART command console (HAL_UART_Receive_IT): status, ack, set threshold, dump log, recalibrate | 4-6 h | communication (named), remote ack | keep ISR to one byte copy |
| 9 | Persistent event log in internal flash | 8-12 h | logging (already claimed by #1) | HIGH: erase stalls CPU, can brick build. SKIP |
| 10 | Sensor health watchdog + loop-overrun counter printed in status line | 2-3 h (counter alone 1 h) | integration robustness EVIDENCE | none, it measures blocking |
| 11 | Step counter / activity level, raise IMPACT_TH 20% while walking | 5-6 h | activity monitoring, context-aware thresholds | competes with tuning time |
| 12 | Jerk + variance features for SUSPECT trigger | 2-3 h | algorithmic change; helps COMPULSORY near-fall marks | changes core trigger → re-run trial table |
| 13 | Replay / self-test mode: stored fall trace through real pipeline | 3-4 h after #1 | robustness/testing | do real drop first, replay second |
| 14 | Periodic heartbeat packet | 1 h | thin | none |

## Ranking by marks / effort
1. #2 Adaptive alpha — "adaptive filtering" by name, uses our own asm in a non-trivial way, great Q&A story about stateless design.
2. #1 Black-box + fall report — logging + communication, gives us the UART plot slide for free.
3. #5 + #6 as a pair — two named rubric words for two states and two timers.
4. #4 Complementary tilt — only idea hitting "sensor fusion" literally; upgrades compulsory architecture row too.
5. #3 Calibration — adaptive thresholds + stronger justification.
Honourable: #12 jerk (helps compulsory near-fall), #10 overrun counter (1 h, hard evidence for non-blocking).
Skip: #9 flash, #14 alone, #13 only as backup, #11 only if time.

## Recommended packages
Package A (safe, ~10 h): #2 + #1 + #5/#6 + overrun counter.
  Story: adaptive filtering → black-box logging → extended state machine with recovery and inactivity.
  Switch mapping: CENTER ack, RIGHT recalibrate/self-test, LEFT mute, UP/DOWN sensitivity.
  Buzzer gains check-in beep + recovered chirp.
Package B (ambitious, ~16 h): Package A with #4 replacing #6, only if core is stable by ~day 8.
  Keep accel-only posture behind a #define fallback.

## Demo notes per idea
- #1: drop on cushion → report scrolls with free-fall dip, impact spike, gyro burst. Sit down → no report.
- #2: print alpha in status line; shake in NORMAL barely moves; drop shows alpha jump to 60 and sharp peak.
- #3: "CAL: rest=1003 mg, noise=12 mg, gyro bias=(..)" ; wiggle during cal → rejected.
- #5: drop → alarm → pick up, hold upright, walk 3 s → RECOVERED, buzzer stops. Drop and leave → LONG_LIE, only CENTER cancels.
- #6: demo timeout 20 s, leave on table → check-in beep, nudge → cancelled, leave → SOS.
- #10: show overruns=0 while buzzer alarms and log dumps.

## UPDATE 16 Sep (after hardware limit + "phone alert" request)
- The 5-way switch is gone: only the Grove buzzer is allowed. All settings (sensitivity,
  mute, self-test, log dump) move to the UART console (#8). CENTER = on-board USER button.
- New headline feature: Telegram alert to the caregiver phone. Three paths compared in
  brainstorm_remote_alert.md (UART gateway / Wi-Fi to laptop relay / Wi-Fi direct TLS).
- New small ideas that came out of it:
  | # | Idea | Effort | Rubric words |
  |---|---|---|---|
  | 15 | Buzzer plays Morse SOS (··· --- ···) in FALL so bystanders recognise it; table-driven pattern player | 1-2 h | alerting |
  | 16 | Escalation ladder: FALL -> caregiver msg; LONG_LIE -> "call ambulance" msg to emergency group, repeat every 30 s until ack | 2 h on top of alert path | Long Lie escalation, SOS |
  | 17 | Fall summary in the message: peak g, peak deg/s, direction (which axis took the impact), time-to-still, severity word | 1 h on top of #7 | logging, communication |
  | 18 | Remote acknowledge from the phone (Telegram button -> relay -> board) | 1 h (UART path) | communication, remote ack |
  | 19 | Boot self-test line: sensors WHO_AM_I, Wi-Fi joined or not, buzzer chirp, overrun=0 | 1 h | integration evidence |
- Final package = Package A (#2 #1 #5 #6 #10) + #15 #16 #17 #18 #19 + the alert path.
  #4 complementary tilt and #3 calibration stay optional.
