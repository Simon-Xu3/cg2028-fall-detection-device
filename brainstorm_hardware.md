# Hardware enhancement candidates (brainstorm result)

## Ground truth found in the starter (matters for core, not just enhancement)
- Accelerometer init = 52 Hz ODR, +-2 g full scale (stm32l4s5i_iot01_accelero.c line 65-67). VERIFIED.
  Our IMPACT_TH ~2000 mg sits exactly at the per-axis clipping ceiling → every impact reads ~2000.
  FIX: after BSP_ACCELERO_Init(), write CTRL1_XL once with 52 Hz + +-4 g (or +-8 g) via SENSOR_IO_Write.
  LSM6DSL_AccReadXYZ re-reads the FS bits each call and rescales, so mg units stay correct.
  Present in slides as "hardware configuration justified by measured impact peaks".
- Gyro = 52 Hz, +-2000 dps. Fine. 20 ms loop is ODR-matched; polling faster returns duplicates.
- No SystemClock_Config in starter → CPU on MSI 4 MHz. Timer prescalers use 4 MHz. UART works.
- HAL modules on: GPIO, EXTI, I2C, DMA, RCC, FLASH, PWR, TIM, UART, SPI, QSPI, DFSDM. ADC and RTC OFF
  (uncomment one #define in stm32l4xx_hal_conf.h if needed).
- QSPI BSP not in starter but exists at Lab/workspace/WiFi_HTTP_Server/Drivers/BSP/B-L4S5I-IOT01/stm32l4s5i_iot01_qspi.c/.h
  and the mx25r6435f component is already in the starter. VERIFIED file exists.
- Two previous-year projects on disk under Lab/ (PA4 buzzer, Wi-Fi dashboard). Reference only, DO NOT COPY:
  plagiarism risk + their moving-average is not this year's EWMA spec. They dropped to 10 Hz sampling for Wi-Fi.

On-board pins taken: PB6/PB7 UART1, PB14 LED2, PC13 button, PB10/PB11 I2C2 sensors, PC10-12+PB12/13+PE0/1/8 Wi-Fi,
PD13/PE6/PA8 BLE, PE10-15 QSPI, PE7/PE9 mics, PD11 LSM6DSL INT1, PC6 ToF. Arduino header pins all free.

## Candidates
| # | Idea | Peripheral / pins | Effort | Difficulty | Rubric words | Risk |
|---|---|---|---|---|---|---|
| 1 | HC-05 Bluetooth SPP → SOS text to caregiver phone (Serial Bluetooth Terminal app) | UART4 on D0/D1 = PA1/PA0, AF8, Transmit_IT | 3-5 h | easy (buy module NOW) | SOS, alerting, communication | none; send on state-entry edges only |
| 2 | QSPI flash black-box: event log + 2 s pre-trigger flight recorder, survives power cycle, dump as CSV | on-board MX25R6435F, PE10-15, copy BSP qspi files | 5-8 h | medium | logging, algorithmic change | erase only in NORMAL on explicit clear; write only on FALL entry |
| 3 | Haptic coin motor, second alert channel for hearing-impaired | D3 = PB0 TIM3_CH3 (same TIM3 as buzzer), NPN + diode, 5 V | 1-2 h | easy | alerting (multi-modal) | none |
| 4 | LPS22HB barometer height-drop witness (advisory, ~6-12 Pa per 0.5-1 m) | on-board, I2C2, BSP_PSENSOR_*, raise ODR to 25 Hz | 3-5 h | medium | SENSOR FUSION, physical units Pa→m | keep advisory, never a gate (drafts) |
| 5 | LDR on ADC → DAY/NIGHT context: faster long-lie at night, inactivity reminder only in day, LED strobe in dark | A0 = PC5 ADC1_IN14, enable HAL_ADC | 2-3 h | easy | adaptive, inactivity, long-lie escalation | none |
| 6 | ST25DV NFC tag as emergency card + last event, readable when MCU dead | on-board, I2C2, BSP_NFCTAG_*, hand-write NDEF | 6-10 h | medium-hard | SOS, communication, logging | 20 ms write on state entry only |
| 7 | RTC wall-clock timestamps, set via UART | on-board LSE, enable HAL_RTC | 2-4 h | easy-medium | supports logging/comm | backup-domain enable gotcha |
| 8 | SSD1306 OLED countdown "Are you OK? 5..4..3 PRESS OK" | I2C1 on D14/D15 = PB9/PB8 | 6-10 h | medium | usability | full frame = 25 ms blocking > 20 ms loop; partial updates or DMA only |
| 9 | HTS221 heat-stress warning | on-board BSP | 2 h | easy | environmental sensing (spec lists it) | none; weak depth |
| 10 | Magnetometer + magnet cradle → NOT_WORN state, no false alarms when knocked off table | on-board BSP_MAGNETO | 2-3 h | easy | state extension, fewer FP | gimmicky risk |
| 11 | LSM6DSL INT1 wake-up + STOP2 sleep | PD11 EXTI | 8-14 h | TRAP | — | kills SysTick/HAL_GetTick |
| 12 | MEMS mics DFSDM → shout SOS / impact thud | PE7/PE9, DMA | 10-16 h | medium-TRAP | sensor fusion, voice SOS | calendar risk, no .ioc |
| 13 | Wi-Fi dashboard | SPI3, es_wifi | 15-25 h | TRAP | — | blocking calls, needs 80 MHz clock, prev group dropped to 10 Hz |
| 14 | On-board BLE | SPI3, no BSP | 30 h+ | TRAP | — | cannot explain every line |
| 15 | VL53L0X ToF | I2C2, no driver | — | TRAP | — | hangs shared I2C2 |

## Ranking (marks per hour)
1. #2 QSPI black box — zero parts, BSP exists, survives power cycle, CSV = slide evidence.
2. #1 HC-05 SOS — cheapest honest wireless, instantly understood demo. Order this week.
3. #5 LDR night mode — ties into long-lie, lets us say "adaptive" truthfully.
4. #3 Haptic — nearly free, accessibility story, not a headline.
5. #4 Barometer fusion — best depth per hour of on-board sensors, keep advisory.
Honourable: #7 RTC if #1 or #2 in. #6 NFC best wow-per-part but NDEF eats a weekend.

## Bundles
A (pick, 12-16 h on top of core): buzzer + 5-way + #2 QSPI + #1 HC-05 + #7 RTC (+ #3 haptic).
  Slide sentence: "Alerting (LED + buzzer + haptic), SOS (button + Bluetooth to caregiver), Long Lie escalation
  (state-machine extension), persistent event logging with pre-trigger flight recorder (QSPI, RTC timestamped)."
  Everything lives in alert/log layer on state-entry edges, nothing inside SUSPECT/CONFIRMING path.
B (parts-free fallback): buzzer + 5-way + #2 QSPI + #4 barometer + #5 LDR.
Do regardless: +-2 g → +-4 g fix.
Do not attempt: #11, #13, #14, #15. #12 only after core frozen.

## UPDATE 16 Sep (teacher's hardware limit: Grove buzzer only)
- #1 HC-05, #3 haptic motor, #5 LDR, #8 OLED are OUT (no external parts). 5-way switch OUT.
- #13 Wi-Fi is NO LONGER a trap. What changed after reading the zip and the lab folder:
  * `es_wifi.c` is inside the starter zip and compiled. `es_wifi_io.c` + `wifi.c` (SPI3 glue)
    are in Lab/workspace/WiFi_HTTP_Server/Common/ = ST's official example, reusable.
  * The "previous group dropped to 10 Hz" problem came from polling the Wi-Fi module inside
    the sample loop. We only touch Wi-Fi at boot and on state entry, so the 20 ms loop is
    untouched except one deliberate pause per alert.
  * Needs SystemClock_Config to 80 MHz (copy from the example). That is also good for the core:
    the BSP I2C timing 0x00702681 is computed for 80 MHz; on the starter's 4 MHz MSI the
    I2C bus runs at ~23 kHz and one accel+gyro read costs ~8-10 ms of the 20 ms budget.
    At 80 MHz it is < 1 ms. Measure before/after for the "problems and solutions" slide.
  * es_wifi.c has ES_WIFI_TCP_SSL_CONNECTION, but ES_WIFI_StartClientConnection only sends
    the remote IP (P3) for plain TCP. Direct-to-Telegram TLS needs that one `if` patched.
  Details and decision: brainstorm_remote_alert.md.
- #2 QSPI black box and #4 barometer stay as optional extras, after the alert path works.
- #14 BLE stays rejected.
