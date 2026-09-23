#include "buzzer.h"
#include "main.h"

/* Morse SOS: dot 100, dash 300, gap 100, letter gap 300, word gap 1000, then repeats.
 * One entry per segment, on/off/on/off ..., starting with the buzzer on. */
static const unsigned int sos_steps[] = {
    100, 100, 100, 100, 100, 300,   /* S: dot gap dot gap dot, letter gap */
    300, 100, 300, 100, 300, 300,   /* O: dash gap dash gap dash, letter gap */
    100, 100, 100, 100, 100, 1000   /* S: dot gap dot gap dot, word gap */
};

/* Same shape as sos_steps, half the duration. */
static const unsigned int sos_fast_steps[] = {
    50, 50, 50, 50, 50, 150,
    150, 50, 150, 50, 150, 150,
    50, 50, 50, 50, 50, 500
};

/* One short beep, then silence. */
static const unsigned int chirp_steps[] = { 150 };

static const unsigned int *steps;   // 0 when nothing is playing
static int step_count;
static int step_index;
static unsigned int step_since;     // when the current step started
static int loops;                   // 1: go back to step 0 at the end, 0: stop

static void set_pin(int on) {
	HAL_GPIO_WritePin(ARD_D6_GPIO_Port, ARD_D6_Pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void play(const unsigned int *new_steps, int count, int loop) {
	steps = new_steps;
	step_count = count;
	loops = loop;
	step_index = 0;
	step_since = HAL_GetTick();
	set_pin(1); // every pattern starts with the buzzer on
}

void buzzer_init(void) {
	GPIO_InitTypeDef gpio = {0};

	__HAL_RCC_GPIOB_CLK_ENABLE();
	gpio.Pin = ARD_D6_Pin;
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(ARD_D6_GPIO_Port, &gpio);

	steps = 0;
	set_pin(0);
}

// change pattern, restarts it from the beginning
void buzzer_set(buzzer_pattern_t p) {
	switch (p) {
	case BUZZER_SOS:
		play(sos_steps, sizeof(sos_steps) / sizeof(sos_steps[0]), 1);
		break;
	case BUZZER_SOS_FAST:
		play(sos_fast_steps, sizeof(sos_fast_steps) / sizeof(sos_fast_steps[0]), 1);
		break;
	case BUZZER_CHIRP:
		play(chirp_steps, sizeof(chirp_steps) / sizeof(chirp_steps[0]), 0);
		break;
	case BUZZER_OFF:
	default:
		steps = 0;
		set_pin(0);
		break;
	}
}

// called every loop pass, advances the pattern by comparing now_ms with the last edge
void buzzer_update(unsigned int now_ms) {
	if (steps == 0) {
		return; // off, or a one-shot pattern that already finished
	}

	if (now_ms - step_since >= steps[step_index]) {
		step_since = now_ms;
		step_index++;

		if (step_index >= step_count) {
			if (!loops) {
				steps = 0; // one-shot pattern done, stay silent
				set_pin(0);
				return;
			}
			step_index = 0;
		}

		set_pin(step_index % 2 == 0); // even index = on, odd = off
	}
}
