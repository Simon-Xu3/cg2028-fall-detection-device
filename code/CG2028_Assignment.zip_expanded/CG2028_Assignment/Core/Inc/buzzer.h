#ifndef BUZZER_H
#define BUZZER_H

/* Grove buzzer, SIG on Arduino D6 = PB1 via the Grove base shield. */

typedef enum { BUZZER_OFF, BUZZER_SOS, BUZZER_SOS_FAST, BUZZER_CHIRP } buzzer_pattern_t;

/* PB1 as GPIO output, off. */
void buzzer_init(void);

/* Change pattern, restarts it from the beginning. */
void buzzer_set(buzzer_pattern_t p);

/* Call every loop pass. Toggles the pin by timestamp, never blocks. */
void buzzer_update(unsigned int now_ms);

#endif
