#ifndef SENSOR_PROCESSING_H
#define SENSOR_PROCESSING_H

/* Units: accel in mg (1000 = 1 g), gyro in dps, time in ms. */

typedef struct {
    int acc_mag_mg;      /* |accel|, about 1000 when the board is still */
    int gyro_mag_dps;    /* |gyro| */
    int tilt_deg;        /* angle between board Z axis and vertical, 0..180 */
    int still;           /* 1 when acc_mag is near 1000 and gyro_mag is small */
} motion_features_t;

/* Reset the six EWMA old_output values to 0. */
void sensor_processing_init(void);

/* Call once every 20 ms with the raw readings.
 * Runs ewma_filter (assembly) on all six axes, then fills *out. */
void sensor_processing_update(const int accel_mg[3], const int gyro_dps[3],
                              motion_features_t *out);

/* Number of samples where the assembly EWMA differed from the C reference. */
int sensor_processing_mismatch_count(void);

/* Write one CSV line "acc_mag,gyro_mag,tilt,still\r\n" into buf. */
void sensor_processing_print(const motion_features_t *f, char *buf, int buf_len);

#endif
