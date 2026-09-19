/*
 * sensor_processing.c
 *
 *  Created on: Sep 19, 2026
 *      Author: Xu Ruijie (A0334914A)
 */

#include "sensor_processing.h"
#include <math.h>
#include <stdio.h>

#define EWMA_ALPHA_ACCEL_PERCENT   25
#define EWMA_ALPHA_GYRO_PERCENT    25

#define STILL_ACC_TOLERANCE_MG  120
#define STILL_GYRO_MAX_DPS       15

extern int ewma_filter(int new_data, int old_output, int alpha_percent);

static int accel_asm_old[3];
static int gyro_asm_old[3];

static int accel_c_old[3];
static int gyro_c_old[3];

static int mismatch_count;

static int vector_magnitude(int x, int y, int z) {
    float x_float = (float)x;
    float y_float = (float)y;
    float z_float = (float)z;

    float result = sqrtf(x_float * x_float
                         + y_float * y_float
                         + z_float * z_float);
    return (int)result;
}

static int calculate_tilt_deg(int x, int y, int z) {
    if ((x == 0) && (y == 0) && (z == 0)) {
        return 0;
    }

    float horizontal = sqrtf((float)x * x + (float)y * y);

    float angle_radians = atan2f(horizontal, (float)z);

    float angle_degrees = angle_radians * (180.0f / 3.14159265f);

    return (int)(angle_degrees + 0.5f);
}

static int ewma_filter_c(int new_data, int old_output, int alpha_percent) {
    return (alpha_percent * new_data
            + (100 - alpha_percent) * old_output) / 100;
}

void sensor_processing_init(void) {
    for (int i = 0; i < 3; i++) {
        accel_asm_old[i] = 0;
        gyro_asm_old[i] = 0;
        accel_c_old[i] = 0;
        gyro_c_old[i] = 0;
    }
    mismatch_count = 0;
}

int sensor_processing_mismatch_count(void) {
    return mismatch_count;
}

void sensor_processing_update(const int accel_mg[3], const int gyro_dps[3],
                              motion_features_t *out) {
    if ((accel_mg == NULL) || (gyro_dps == NULL) || (out == NULL)) {
        return;
    }

    int accel_asm_filtered[3];
    int gyro_asm_filtered[3];
    int accel_c_filtered[3];
    int gyro_c_filtered[3];
    int mismatch_this_sample = 0;

    for (int i = 0; i < 3; i++) {
        accel_asm_filtered[i] = ewma_filter(accel_mg[i], accel_asm_old[i],
                                             EWMA_ALPHA_ACCEL_PERCENT);
        accel_asm_old[i] = accel_asm_filtered[i];

        accel_c_filtered[i] = ewma_filter_c(accel_mg[i], accel_c_old[i],
                                             EWMA_ALPHA_ACCEL_PERCENT);
        accel_c_old[i] = accel_c_filtered[i];

        gyro_asm_filtered[i] = ewma_filter(gyro_dps[i], gyro_asm_old[i],
                                            EWMA_ALPHA_GYRO_PERCENT);
        gyro_asm_old[i] = gyro_asm_filtered[i];

        gyro_c_filtered[i] = ewma_filter_c(gyro_dps[i], gyro_c_old[i],
                                            EWMA_ALPHA_GYRO_PERCENT);
        gyro_c_old[i] = gyro_c_filtered[i];

        if ((accel_asm_filtered[i] != accel_c_filtered[i]) ||
            (gyro_asm_filtered[i] != gyro_c_filtered[i])) {
            mismatch_this_sample = 1;
        }
    }

    if (mismatch_this_sample) {
        mismatch_count++;
    }

    out->acc_mag_mg = vector_magnitude(accel_asm_filtered[0],
                                        accel_asm_filtered[1],
                                        accel_asm_filtered[2]);
    out->gyro_mag_dps = vector_magnitude(gyro_asm_filtered[0],
                                          gyro_asm_filtered[1],
                                          gyro_asm_filtered[2]);
    out->tilt_deg = calculate_tilt_deg(accel_asm_filtered[0],
                                        accel_asm_filtered[1],
                                        accel_asm_filtered[2]);

    int acceleration_error = out->acc_mag_mg - 1000;

    if (acceleration_error < 0) {
        acceleration_error = -acceleration_error;
    }

    if ((acceleration_error <= STILL_ACC_TOLERANCE_MG) &&
        (out->gyro_mag_dps <= STILL_GYRO_MAX_DPS)) {
        out->still = 1;
    } else {
        out->still = 0;
    }
}

void sensor_processing_print(const motion_features_t *f,
                             char *buf, int buf_len) {
    if ((f == NULL) || (buf == NULL) || (buf_len <= 0)) {
        return;
    }

    snprintf(buf, (size_t)buf_len, "%d,%d,%d,%d\r\n",
             f->acc_mag_mg,
             f->gyro_mag_dps,
             f->tilt_deg,
             f->still);
}
