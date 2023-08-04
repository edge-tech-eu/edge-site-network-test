#include "Particle.h"

unsigned long timeout_delta;
unsigned long now_delta;

void timeouttimer_set(unsigned long time) {

    Log.info("timout set for %d",(int) time);

    unsigned long now = millis();

    now_delta = 0;
    timeout_delta = now + time;

    if(timeout_delta < now) {

        now_delta += time;
        timeout_delta += time;
    }
}

bool timeouttimer_expired() {

    if(timeout_delta < millis() + now_delta) {

        Log.info("timed out");

        return(true);
    }

    return(false);
}