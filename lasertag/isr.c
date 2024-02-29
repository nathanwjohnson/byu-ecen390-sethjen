#include "isr.h"
#include "trigger.h"
#include "hitLedTimer.h"
#include "lockoutTimer.h"

// The interrupt service routine (ISR) is implemented here.
// Add function calls for state machine tick functions and
// other interrupt related modules.

// Perform initialization for interrupt and timing related modules.
void isr_init() {
    trigger_init();
    hitLedTimer_init();
    lockoutTimer_init();
}

// This function is invoked by the timer interrupt at 100 kHz.
void isr_function() {
    trigger_tick();
    hitLedTimer_tick();
    lockoutTimer_tick();
}
