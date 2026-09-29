# ElderCare Fall Detection Band — CG2028

This project contains the STM32 firmware and the laptop Telegram relay.

## Contents

- `Core/Src/mov_avg.s`: ARM assembly EWMA filter.
- `Core/Src/sensor_processing.c`: filtering and motion features.
- `Core/Src/fall_detector.c`: fall detection and long-lie escalation.
- `Core/Src/status_indicator.c`: LED and user-button handling.
- `Core/Src/buzzer.c`: audible SOS patterns.
- `Core/Src/main.c`: sensor acquisition, main loop, serial data and alerts.
- `Core/Inc/`: corresponding headers.
- `Drivers/`, startup code, linker scripts and IDE configuration: board support.
- `tools/`: Python Telegram relay, dependency list and configuration example.

## Build and run

Import this directory into STM32CubeIDE using Existing Projects into Workspace.
Build `CG2028_Assignment` and flash the B-L4S5I-IOT01A board through ST-LINK.
The firmware reports serial data at 115200 baud. The Grove buzzer uses D6 (PB1).
The user button acknowledges a fall alarm.

For Telegram alerts, follow [tools/README.md](tools/README.md). The laptop relay
must be running and connected to the board; flashing the firmware alone does
not send Telegram messages.

## Export for submission

Refresh the project in STM32CubeIDE after external file changes. Use File >
Export > General > Archive File, select this project and retain its directory
structure. Include `Core`, `Drivers`, `tools`, this README, linker scripts,
`.project`, `.cproject`, `.settings`, and the debug/launch configuration files.
Generated `Debug` and `Release` folders can be omitted.

If you created `tools/secrets.py`, deselect it in the export dialog. Include only
`tools/secrets.py.example` for configuration; Git ignore rules do not prevent
IDE archive export from including private files.

The independent `CG2028_Assignment_Test` project runs the EWMA open test cases.
It remains a separate project and is not included when exporting only this
main project. Slides and test evidence also remain outside this directory;
submit those separately if required by the assignment instructions.
