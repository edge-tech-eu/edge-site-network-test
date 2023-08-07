 #include "Particle.h"
 
/* system_defs.h

    RESET_REASON_NONE = 0, ///< Invalid reason code.
    RESET_REASON_UNKNOWN = 10, ///< Unspecified reason.
    // Hardware
    RESET_REASON_PIN_RESET = 20, ///< Reset from the reset pin.
    RESET_REASON_POWER_MANAGEMENT = 30, ///< Low-power management reset.
    RESET_REASON_POWER_DOWN = 40, ///< Power-down reset.
    RESET_REASON_POWER_BROWNOUT = 50, ///< Brownout reset.
    RESET_REASON_WATCHDOG = 60, ///< Watchdog reset.
    // Software
    RESET_REASON_UPDATE = 70, ///< Reset to apply firmware update.
    RESET_REASON_UPDATE_ERROR = 80, ///< Generic firmware update error (deprecated).
    RESET_REASON_UPDATE_TIMEOUT = 90, ///< Firmware update timeout.
    RESET_REASON_FACTORY_RESET = 100, ///< Factory reset requested.
    RESET_REASON_SAFE_MODE = 110, ///< Safe mode requested.
    RESET_REASON_DFU_MODE = 120, ///< DFU mode requested.
    RESET_REASON_PANIC = 130, ///< System panic.
    RESET_REASON_USER = 140 ///< User-requested reset.
*/

void reset_reason_log() {

    int  reset_reason_value = System.resetReason();

    switch(reset_reason_value) {

        case RESET_REASON_UPDATE:

            Log.info("Reset: Flashed successfully");
            break;

        case RESET_REASON_PIN_RESET:

            Log.info("Reset: Reset pin pressed");
            break;

        case RESET_REASON_UNKNOWN:

            Log.info("Reset: Unknown, Long wait to allow flashing");

            // cloud flash safety
            delay(300000);

            break;

        case RESET_REASON_USER:

            Log.info("Reset: software/user");
            break;

        case RESET_REASON_WATCHDOG:

            Log.info("Reset: Watchdog");
            break;

        default:

            Log.info("Reset: reason: %d", reset_reason_value);
            break;
    }
}