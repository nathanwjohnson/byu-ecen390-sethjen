/*
This software is provided for student assignment use in the Department of
Electrical and Computer Engineering, Brigham Young University, Utah, USA.
Users agree to not re-host, or redistribute the software, in source or binary
form, to other persons or other institutions. Users may modify and use the
source code for personal or educational use.
For questions, contact Brad Hutchings or Jeff Goeders, https://ece.byu.edu/
*/

// Uncomment to run tests, various Milestones
// #define RUNNING_MODE_TESTS

// Uncomment to run Milestone 3, Task 2
// #define RUNNING_MODE_M3_T2

// Uncomment to run continuous/shooter mode, Milestone 3, Task 3
// #define RUNNING_MODE_M3_T3

// Uncomment to run two-player mode, Milestone 5
// #define RUNNING_MODE_M5

#include <assert.h>
#include <stdio.h>

#include "bufferTest.h"
#include "buttons.h"
#include "detector.h"
#include "display.h"
#include "filter.h"
#include "filterTest.h"
#include "game.h"
#include "hitLedTimer.h"
#include "interrupts.h"
#include "isr.h"
#include "leds.h"
#include "lockoutTimer.h"
#include "mio.h"
#include "runningModes.h"
#include "sound.h"
#include "switches.h"
#include "transmitter.h"
#include "trigger.h"
#include "queueTest.h"

int main() {
  mio_init(false);  // true enables debug prints
  leds_init(false); // true enables debug prints
  buttons_init();
  switches_init();
  display_init();
  display_fillScreen(DISPLAY_BLACK);
  display_println("System is Alive");

#ifdef RUNNING_MODE_TESTS
  // interrupts not needed for these tests
  queue_runTest(); // M1
  // filter_runTest(); // M3 T1
  // transmitter_runTest(); // M3 T2
  // buffer_runTest(); // M3 T3
  // detector_runTest(); // M3 T3
  // sound_runTest(); // M5
#endif

#ifdef RUNNING_MODE_M3_T2
  // add transmitter, trigger, hitLedTimer, lockoutTimer,
  // sound, and buffer init functions to isr_init(),
  // i.e. anything with _tick() functions.
  isr_init();

  interrupts_initAll(false);          // main interrupt init function.
  interrupts_enableTimerGlobalInts(); // enable global interrupts.
  interrupts_startArmPrivateTimer();  // start the main timer.
  interrupts_enableArmInts(); // now the ARM processor can see interrupts.

  transmitter_runTestNoncontinuous();
  transmitter_runTestContinuous();
  trigger_runTest();
  hitLedTimer_runTest();
  lockoutTimer_runTest();
#endif

#ifdef RUNNING_MODE_M3_T3
  // The program comes up in continuous mode by default.
  // Hold BTN2 while the program starts to come up in shooter mode.
  // Interrupts are enabled in runningModes.
  if (buttons_read() & BUTTONS_BTN2_MASK) {
    printf("Starting shooter mode\n");
    runningModes_shooter(); // Run shooter mode if BTN2 is depressed.
  } else {
    printf("Starting continuous mode\n");
    runningModes_continuous(); // Otherwise, go to continuous mode.
  }
#endif

#ifdef RUNNING_MODE_M5
  // No printf here since board not likely connected to host with USB
  game_twoTeamTag();
#endif

  display_println("System is Ending");
  return 0;
}
