#ifndef RESET_REASON_H
#define RESET_REASON_H

#include "Particle.h"

#define RESET_USER_ENABLE_ETHERNET  100
#define RESET_USER_DISABLE_ETHERNET 101
#define RESET_USER_FACTORY_DEFAULTS 110
#define RESET_USER_NO_VALID_METER   120
#define RESET_USER_TEST_MODE        130

void reset_reason_log(void);

#endif