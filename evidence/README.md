# Test evidence

Serial logs recorded from the board (115200 baud) and phone screenshots. Every CSV row is
one line printed by the firmware: `state,acc_mg,gyro_dps,tilt_deg,still` every 500 ms, plus
`ALERT,...` lines the moment a fall is confirmed, escalates, or is acknowledged.

| File | Date | What it shows |
|---|---|---|
| `2026-09-20_real_fall.csv` | 20 Sep, case off, board slammed onto a sofa and left on its side | `NORMAL -> SUSPECT -> CONFIRMING -> FALL` in 4.5 s (rows 10-18, FALL at 8.8 s). Has a `t_s` column (seconds since recording start). Recording ends when the USB cable was pulled out. |
| `2026-09-23_longlie_untouched.csv` | 23 Sep, E1 firmware | Fall at tick 178141, board not touched, `ALERT,LONG_LIE,208141,moved=0` exactly 30 s later. The header line and `SUSPECT` rows after it are a board reset (the black RESET button was pressed instead of the blue USER button). |
| `2026-09-23_longlie_nudged.csv` | 23 Sep, E1 firmware | Fall at tick 16961, board nudged once (`FALL,996,18,84,0`), `ALERT,LONG_LIE,46961,moved=1`, then `ALERT,ACK` from the blue button and back to `NORMAL`. First 480 rows are an earlier attempt. |
| `2026-09-24_false_positive_walk_pocket_carry.csv` | 24 Sep, second board, main firmware | Walking 20 steps with the board in hand, in a pocket, carried flat and turning around. 234 `NORMAL` rows, 6 `SUSPECT` rows in three sub-second blips that returned to `NORMAL` on their own, no `CONFIRMING`/`FALL`. Rows have a `t_s` column. |
| `2026-09-24_false_positive_flip_shake_pickup.csv` | 24 Sep | Flipping the board 360 degrees in hand, light shaking for 3 s, picking up and putting down three times. All rows `NORMAL`. |
| `2026-09-24_near_fall_sit_bend_lie_down.csv` | 24 Sep | Sitting down hard, bending to tie a shoe, lying down slowly on a sofa for 5 s (tilt reached 130 degrees), medium-speed bend, squat and stand. All rows `NORMAL`: no impact above 2000 mg, so `SUSPECT` was never entered even while lying down. |
| `2026-09-23_telegram_all_four_messages.jpg` | 23 Sep, E1 + E3 | Telegram group "CG2028 Enhancement Test" receiving all four messages from `tools/relay.py`: FALL, LONG_LIE with no movement (Call 995 + address), ACK; then FALL, LONG_LIE with movement, ACK. |

How the CSVs were recorded (macOS Terminal, no extra software):
```
exec 3<>/dev/cu.usbmodemXXXX
stty -f /dev/cu.usbmodemXXXX 115200 raw
cat <&3 | tee my_test.csv
```
