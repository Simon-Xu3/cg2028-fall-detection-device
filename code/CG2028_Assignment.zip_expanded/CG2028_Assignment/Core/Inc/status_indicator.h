#ifndef STATUS_INDICATOR_H
#define STATUS_INDICATOR_H

#include "fall_detector.h"

/* LED2 and the blue USER button (PC13). */
void status_indicator_init(void);

/* Call every loop. Blinks LED2 by timestamp, never blocks. */
void status_indicator_update(fall_state_t state, unsigned int now_ms);

/* Returns 1 once per button press, 0 otherwise. */
int status_indicator_button_pressed(void);

#endif
