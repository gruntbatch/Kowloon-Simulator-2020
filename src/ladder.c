#include "ladder.h"


struct Rung {
    Up up;
    Down down;
};


#define MAX_RUNG_COUNT 64
static int rung_count = 0;
static struct Rung rungs[MAX_RUNG_COUNT];


void AddRung(Up up, Down down) {
    if (rung_count < MAX_RUNG_COUNT) {
	rungs[rung_count].up = up;
	rungs[rung_count].down = down;
	rung_count++;
    }
}


int Climb(void) {
    if (rung_count < MAX_RUNG_COUNT) {
	struct Rung* rung = rungs;

	for (int i=0; i<rung_count; i++) {
	    if (rung->up && rung->up() == UP) {
		rung++;
	    } else {
		break;
	    }
	}

	for (; rungs <= rung;) {
	    if (rung->down) {
		rung->down();
	    }
	    rung--;
	}
	return 0;
    } else {
	return -1;
    }
}
