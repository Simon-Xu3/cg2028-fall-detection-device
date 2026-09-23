#include "fall_detector.h"

/* Thresholds, tune on the real board with the case on. */
#define FREEFALL_MG          500
#define IMPACT_MG            2000
#define ROTATION_DPS         200
#define LYING_TILT_DEG       60
#define SUSPECT_TIMEOUT_MS   1000
#define CONFIRM_STILL_MS     2000
#define CONFIRM_TIMEOUT_MS   5000
#define LONG_LIE_MS         30000   // no ack for 30 s after a fall -> escalate
#define MOVED_MS              300   // must be not-still this long to count as movement

static fall_state_t state;
static unsigned int state_since; // enter current state
static int moved_since_fall;     // real movement seen while in FALL
static unsigned int not_still_since; // when the current not-still stretch began

void fall_detector_init(void) {
	state = STATE_NORMAL;
	state_since = 0;
	moved_since_fall = 0;
}

void fall_detector_ack(void) {
	// in FALL state it works
    if (state == STATE_FALL || state == STATE_LONG_LIE) {
        state = STATE_NORMAL;
    }
}

int fall_detector_moved_since_fall(void) {
	return moved_since_fall;
}

const char* fall_state_name(fall_state_t s) {
	switch(s) {
		case STATE_NORMAL:     return "NORMAL";
	    case STATE_SUSPECT:    return "SUSPECT";
	    case STATE_CONFIRMING: return "CONFIRMING";
	    case STATE_FALL:       return "FALL";
	    case STATE_LONG_LIE:   return "LONG_LIE";
	    default:               return "?";
	}
}

// Jump to next state
static void jump(fall_state_t next, unsigned int now_ms) {
    state = next;
    state_since = now_ms;
}

// called every 20 ms, each case: check next step, else check timeout
fall_state_t fall_detector_update(const motion_features_t *f, unsigned int now_ms) {
	unsigned int passed_time = now_ms - state_since; // how long in this state

	switch (state) {
	case STATE_NORMAL:
		// too small = free fall, too big = impact
		if (f->acc_mag_mg < FREEFALL_MG || f->acc_mag_mg > IMPACT_MG) {
			jump(STATE_SUSPECT, now_ms);
		}
		break;

	case STATE_SUSPECT:
		// body rotated -> keep going, no rotation in 1 s -> near fall, give up
		if (f->gyro_mag_dps > ROTATION_DPS) {
			jump(STATE_CONFIRMING, now_ms);
		} else if (passed_time > SUSPECT_TIMEOUT_MS) {
			jump(STATE_NORMAL, now_ms);
		}
		break;

	case STATE_CONFIRMING:
		// after 2 s the person must be lying still -> fall, 3 s passed -> got up
		if (passed_time >= CONFIRM_STILL_MS && f->tilt_deg > LYING_TILT_DEG && f->still) {
			jump(STATE_FALL, now_ms);
			moved_since_fall = 0;
			not_still_since = now_ms;
		} else if (passed_time > CONFIRM_TIMEOUT_MS) {
			jump(STATE_NORMAL, now_ms);
		}
		break;

	case STATE_FALL:
		// a single noisy sample is not movement: the person must be
		// not-still for MOVED_MS in a row before we say they moved
		if (f->still) {
			not_still_since = now_ms;
		} else if (now_ms - not_still_since >= MOVED_MS) {
			moved_since_fall = 1;
		}
		// nobody pressed the button for 30 s -> long lie, escalate
		if (passed_time >= LONG_LIE_MS) {
			jump(STATE_LONG_LIE, now_ms);
		}
		break;

	case STATE_LONG_LIE:
		// stay here until button ack
		break;
	}

	return state;
}

