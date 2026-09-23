#include "status_indicator.h"
#include "../../Drivers/BSP/B-L4S5I-IOT01/stm32l4s5i_iot01.h"

#define SLOW_BLINK_MS   500
#define FAST_BLINK_MS   100

static unsigned int last_toggle; // when LED last changed
static int last_button;          // button reading last time, 1 = down

void status_indicator_init(void) {
	BSP_LED_Init(LED2);
	BSP_LED_Off(LED2);
	BSP_PB_Init(BUTTON_USER, BUTTON_MODE_GPIO);
	last_toggle = 0;
	last_button = 0;
}

// slow blink normally, fast blink after a fall
void status_indicator_update(fall_state_t state, unsigned int now_ms) {
	unsigned int period;

	if (state == STATE_FALL || state == STATE_LONG_LIE) {
		period = FAST_BLINK_MS;
	} else {
		period = SLOW_BLINK_MS;
	}

	if (now_ms - last_toggle >= period) {
		BSP_LED_Toggle(LED2);
		last_toggle = now_ms;
	}
}

// 1 only at the moment the button goes down, so holding it counts once
int status_indicator_button_pressed(void) {
	int down = (BSP_PB_GetState(BUTTON_USER) == 0); // pull-up, pressed reads 0
	int pressed = down && !last_button;

	last_button = down;
	return pressed;
}
