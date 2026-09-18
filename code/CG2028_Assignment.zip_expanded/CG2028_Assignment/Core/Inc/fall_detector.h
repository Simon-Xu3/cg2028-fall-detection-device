#ifndef FALL_DETECTOR_H
#define FALL_DETECTOR_H

#include "sensor_processing.h"

typedef enum {
    STATE_NORMAL,        /* LED slow blink */
    STATE_SUSPECT,       /* free-fall or impact seen, waiting for rotation */
    STATE_CONFIRMING,    /* impact + rotation seen, checking for stillness */
    STATE_FALL           /* confirmed, LED on until acknowledged */
} fall_state_t;

void fall_detector_init(void);

/* Call once every 20 ms after sensor_processing_update. Returns the new state. */
fall_state_t fall_detector_update(const motion_features_t *f, unsigned int now_ms);

/* USER button pressed: go back to STATE_NORMAL. */
void fall_detector_ack(void);

/* "NORMAL", "SUSPECT", ... for the UART output. */
const char *fall_state_name(fall_state_t s);

#endif
