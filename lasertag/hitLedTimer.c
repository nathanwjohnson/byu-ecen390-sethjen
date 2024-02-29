#include "hitLedTimer.h"
#include "include/leds.h"
#include "include/mio.h"
#include "utils.h"

// The hitLedTimer is active for 1/2 second once it is started.
// While active, it turns on the LED connected to MIO pin 11
// and also LED LD0 on the ZYBO board.

#define LED_1 1      // used to turn LED1 on

volatile static bool isEnabled;
volatile static bool shouldStart;
volatile static uint64_t ticks;

// States for the controller state machine.
enum hitLedTimer_st_t {
	init_st,           // hit LED off
	running_st         // hit LED on for 1/2 second
};
volatile static enum hitLedTimer_st_t currentState;

// Need to init things.
void hitLedTimer_init() {
    isEnabled = false;
    ticks = 0;
    currentState = init_st;
    shouldStart = false;
    mio_setPinAsOutput(HIT_LED_TIMER_OUTPUT_PIN);
}

// Standard tick function.
void hitLedTimer_tick() {

    if (!isEnabled) {
        return;
    }
    // State updates
    switch(currentState) {
        case init_st:
            if (shouldStart) {
                ticks = 0;
                currentState = running_st;
                hitLedTimer_turnLedOn();
            }
            break;
        case running_st:
            if (ticks >= HIT_LED_TIMER_EXPIRE_VALUE) {
                ticks = 0;
                shouldStart = false;
                hitLedTimer_turnLedOff();
                currentState = init_st;
            }
            break;
        default:
            //error
            break;
    }
    
    // State actions
    switch(currentState) {
        case init_st:
            break;
        case running_st:
            ticks++;
            break;
        default:
            //error
            break;
    }  
}

// Calling this starts the timer.
void hitLedTimer_start() {
    shouldStart = true;
}

// Returns true if the timer is currently running.
bool hitLedTimer_running() {
    if (currentState == running_st) {
        return true;
    } else {
        return false;
    }
}

// Turns the gun's hit-LED on.
void hitLedTimer_turnLedOn() {
    leds_write(LED_1);
    mio_writePin(HIT_LED_TIMER_OUTPUT_PIN, 1);
}

// Turns the gun's hit-LED off.
void hitLedTimer_turnLedOff() {
    leds_write(0);
    mio_writePin(HIT_LED_TIMER_OUTPUT_PIN, 0);
}

// Disables the hitLedTimer.
void hitLedTimer_disable() {
    isEnabled = false;
}

// Enables the hitLedTimer.
void hitLedTimer_enable() {
    isEnabled = true;
}

// Runs a visual test of the hit LED until BTN3 is pressed.
// The test continuously blinks the hit-led on and off.
// Depends on the interrupt handler to call tick function.
void hitLedTimer_runTest() {
    hitLedTimer_enable();
    while (true) {
        hitLedTimer_start(); // Step 1

        // Step 2: Wait until hitLedTimer_running() is false
        while (hitLedTimer_running()) {
            // Do nothing, just wait
        }

        utils_msDelay(300); // Step 3: Delay for 300 ms using utils_msDelay()
    }
}
