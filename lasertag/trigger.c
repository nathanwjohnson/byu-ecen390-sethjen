#include "trigger.h"
#include <stdbool.h>
#include <stdio.h>
#include "include/mio.h"
#include "drivers/buttons.h"

// Uncomment for debug prints
#define DEBUG
 
#if defined(DEBUG)
#include <stdio.h>
#include "xil_printf.h"
#define DPRINTF(...) printf(__VA_ARGS__)
#define DPCHAR(ch) outbyte(ch)
#else
#define DPRINTF(...)
#define DPCHAR(ch)
#endif

// The trigger state machine debounces both the press and release of gun
// trigger. Ultimately, it will activate the transmitter when a debounced press
// is detected.

#define TRIGGER_GUN_TRIGGER_MIO_PIN 10
#define GUN_TRIGGER_PRESSED 1
#define MAX_TICKS 5

volatile static bool ignoreGunInput;
volatile static bool isEnabled;
volatile static trigger_shotsRemaining_t shotsRemaining;
volatile static uint64_t ticks = 0;

// States for the controller state machine.
enum trigger_st_t {
	released_st,       // Wait here until the button has been pressed continuously for 50ms
	pressed_st         // Wait here until the button has been released continuously for 50ms
};
volatile static enum trigger_st_t currentState;

// Trigger can be activated by either btn0 or the external gun that is attached to TRIGGER_GUN_TRIGGER_MIO_PIN
// Gun input is ignored if the gun-input is high when the init() function is invoked.
bool triggerPressed() {
	return ((!ignoreGunInput & (mio_readPin(TRIGGER_GUN_TRIGGER_MIO_PIN) == GUN_TRIGGER_PRESSED)) || 
                (buttons_read() & BUTTONS_BTN0_MASK));
}

// Init trigger data-structures.
// Initializes the mio subsystem.
// Determines whether the trigger switch of the gun is connected
// (see discussion in lab web pages).
void trigger_init() {

    isEnabled = false;
    shotsRemaining = 5;
    currentState = released_st;
    ticks = 0;

    mio_setPinAsInput(TRIGGER_GUN_TRIGGER_MIO_PIN);
    // If the trigger is pressed when trigger_init() is called, assume that the gun is not connected and ignore it.
    if (triggerPressed()) {
        ignoreGunInput = true;
    }
}

// Standard tick function.
void trigger_tick() {
    if (isEnabled) {
        // State updates
        switch(currentState) {
            case released_st:
                printf("released state1\n");
                if (ticks >= MAX_TICKS) {
                    printf("D\n");
                    ticks = 0;
                    currentState = pressed_st;
                }
                break;
            case pressed_st:
                printf("pressed state1\n");
                if (ticks >= MAX_TICKS) {
                    printf("U\n");
                    ticks = 0;
                    currentState = released_st;
                }
                break;
            default:
                // print an error message here.
                break;
        }
        
        // State actions
        switch(currentState) {
            case released_st:
                printf("released state2\n");
                if (triggerPressed()) {
                    printf("pressed\n");
                    ticks++;
                } else {
                    ticks = 0;
                }
                break;
            case pressed_st:
                printf("pressed state2\n");
                if (!triggerPressed()) {
                    printf("not pressed\n");
                    ticks++;
                } else {
                    ticks = 0;
                }
                break;
            default:
                // print an error message here.
                break;
        }  
    }
}

// Enable the trigger state machine. The trigger state-machine is inactive until
// this function is called. This allows you to ignore the trigger when helpful
// (mostly useful for testing).
void trigger_enable() {
    isEnabled = true;
}

// Disable the trigger state machine so that trigger presses are ignored.
void trigger_disable() {
    isEnabled = false;
}

// Returns the number of remaining shots.
trigger_shotsRemaining_t trigger_getRemainingShotCount() {
    return shotsRemaining;
}

// Sets the number of remaining shots.
void trigger_setRemainingShotCount(trigger_shotsRemaining_t count) {
    shotsRemaining = count;
}

// Runs the test continuously until BTN3 is pressed.
// The test just prints out a 'D' when the trigger or BTN0
// is pressed, and a 'U' when the trigger or BTN0 is released.
// Depends on the interrupt handler to call tick function.
void trigger_runTest() {
    printf("starting trigger_runTest()\n");
    trigger_enable();
}
