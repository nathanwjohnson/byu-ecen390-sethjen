#include "lockoutTimer.h"
#include <stdio.h>
#include "include/mio.h"
#include "drivers/intervalTimer.h"
#include <time.h>

// The lockoutTimer is active for 1/2 second once it is started.
// It is used to lock-out the detector once a hit has been detected.
// This ensures that only one hit is detected per 1/2-second interval.

volatile static uint64_t ticks = 0;
volatile static bool shouldStart;

// States for the controller state machine.
enum lockoutTimer_st_t {
	waiting_st,       // Wait here until hit detected
	locked_st         // locked out for 1/2 second
};
volatile static enum lockoutTimer_st_t currentState;

// Perform any necessary inits for the lockout timer.
void lockoutTimer_init() {
    ticks = 0;
    shouldStart = false;
    currentState = waiting_st;
}

// Standard tick function.
void lockoutTimer_tick() {
    // State updates
    switch(currentState) {
        case waiting_st:
            break;
        case locked_st:
            // After waiting half a second, move back to the waiting state
            if (ticks >= LOCKOUT_TIMER_EXPIRE_VALUE) {
                ticks = 0;
                shouldStart = false;
                currentState = waiting_st;
            }
            break;
        default:
            //error
            break;
    }
    
    // State actions
    switch(currentState) {
        case waiting_st:
            break;
        case locked_st:
            ticks++;
            break;
        default:
            //error
            break;
    }  
}

// Calling this starts the timer.
void lockoutTimer_start() {
    currentState = locked_st;
    ticks = 0;
}

// Returns true if the timer is running.
bool lockoutTimer_running() {
    return (currentState == locked_st) ? true : false;
}

// Test function assumes interrupts have been completely enabled and
// lockoutTimer_tick() function is invoked by isr_function().
// Prints out pass/fail status and other info to console.
// Returns true if passes, false otherwise.
// This test uses the interval timer to determine correct delay for
// the interval timer.
bool lockoutTimer_runTest() {
    
    // Start an interval timer,
    intervalTimer_start(1); //TODO remove this for milestone 3
    // utils_msDelay(100);

    // Invoke lockoutTimer_start(),
    lockoutTimer_start();

    // Wait while lockoutTimer_running() is true (another while-loop),
    while(lockoutTimer_running()) {
    }
    // printf("                 hi\n");
    // Once lockoutTimer_running() is false, stop the interval timer,
    intervalTimer_stop(1);

    // Print out the time duration from the interval timer.
    printf("Lockout timer duration: %.3f seconds\n", intervalTimer_getTotalDurationInSeconds(1));

    return true;
}
