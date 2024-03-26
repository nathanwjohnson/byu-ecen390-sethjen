/*
This software is provided for student assignment use in the Department of
Electrical and Computer Engineering, Brigham Young University, Utah, USA.
Users agree to not re-host, or redistribute the software, in source or binary
form, to other persons or other institutions. Users may modify and use the
source code for personal or educational use.
For questions, contact Brad Hutchings or Jeff Goeders, https://ece.byu.edu/
*/

/*
The code in runningModes.c can be an example for implementing the game here.
*/

#include <stdio.h>

#include "detector.h"
#include "filter.h"
#include "hitLedTimer.h"
#include "interrupts.h"
#include "intervalTimer.h"
#include "invincibilityTimer.h"
#include "runningModes.h"
#include "sound/sound.h"
#include "switches.h"
#include "trigger.h"

#define STARTING_LIVES 3
#define STARTING_BULLETS 10
#define HITS_PER_LIFE 5
#define TEAM_A_FREQUENCY 6
#define TEAM_B_FREQUENCY 9

#define DETECTOR_HIT_ARRAY_SIZE \
  FILTER_FREQUENCY_COUNT // The array contains one location per user frequency.

// Defined to make things more readable.
#define INTERRUPTS_CURRENTLY_ENABLED true
#define INTERRUPTS_CURRENTLY_DISABLE false

#define ISR_CUMULATIVE_TIMER INTERVAL_TIMER_TIMER_0 // Used by the ISR.
#define RELOAD_TRIGGER_TIMER \
  INTERVAL_TIMER_TIMER_1 // Used to compute total run-time.
#define RELOAD_TRIGGER_LENGTH_S 3

volatile static uint8_t bulletsLeft = STARTING_BULLETS;

// Reload the gun by resetting number of bullets in the gun and playing a reload sound
void reload() {
  bulletsLeft = STARTING_BULLETS;
  // TODO: Play a reload sound
}

// This game supports two teams, Team-A and Team-B.
// Each team operates on its own configurable frequency.
// Each player has a fixed set of lives and once they
// have expended all lives, operation ceases and they are told
// to return to base to await the ultimate end of the game.
// The gun is clip-based and each clip contains a fixed number of shots
// that takes a short time to reload a new clip.
// The clips are automatically loaded.
// Runs until BTN3 is pressed.
void game_twoTeamTag(void) {
  uint16_t hitCount = 0;
  runningModes_initAll();
  sound_setVolume(sound_maximumVolume_e);
  sound_startSound();

  // Configuration...

  uint8_t livesLeft = STARTING_LIVES;
  bool gameOver = false;

  bool reloadTriggerTimerRunning = false;

  // Init the ignored-frequencies so no frequencies are ignored.
  bool ignoredFrequencies[FILTER_FREQUENCY_COUNT];
  for (uint16_t i = 0; i < FILTER_FREQUENCY_COUNT; i++)
    ignoredFrequencies[i] = true;

  detector_setIgnoredFrequencies(ignoredFrequencies);

  // read switch 0 to determine team membership
  uint16_t switchSetting = switches_read() & SWITCHES_SW0_MASK; // Bit-mask the results.

  if (switchSetting) { // Team B

  } else { // Team A
  }

  trigger_enable();                          // Makes the state machine responsive to the trigger.
  interrupts_enableTimerGlobalInts();        // Allow timer interrupts.
  interrupts_startArmPrivateTimer();         // Start the private ARM timer running.
  interrupts_enableArmInts();                // ARM will now see interrupts after this.
  lockoutTimer_start();                      // Ignore erroneous hits at startup (when all power
                                             // values are essentially 0).
  intervalTimer_reset(RELOAD_TRIGGER_TIMER); // Used to measure main-loop execution time.

  // Implement game loop...
  while (!gameOver) { // Run until you detect BTN3 pressed.

    // Run filters, compute power, run hit-detection.
    detector(INTERRUPTS_CURRENTLY_ENABLED); // Interrupts are currently enabled.

    // If there is a hit detected, handle it
    if (detector_hitDetected()) { // Hit detected
      hitCount++;                 // increment the hit count.
      detector_clearHit();        // Clear the hit.
      detector_hitCount_t
          hitCounts[DETECTOR_HIT_ARRAY_SIZE]; // Store the hit-counts here.
      detector_getHitCounts(hitCounts);       // Get the current hit counts.
      histogram_plotUserHits(hitCounts);      // Plot the hit counts on the TFT.
    }

    // If the trigger is pressed, start a timer
    if (trigger_isPressed()) {
      if (!reloadTriggerTimerRunning) {
        intervalTimer_start(RELOAD_TRIGGER_TIMER);
        reloadTriggerTimerRunning = true;
      }
    } else if (reloadTriggerTimerRunning) {
      // if the trigger isn't pressed but the trigger timer is still running, stop and reset the timer.
      intervalTimer_stop(RELOAD_TRIGGER_TIMER);
      reloadTriggerTimerRunning = false;
      intervalTimer_reset(RELOAD_TRIGGER_TIMER);
    }

    // if the trigger has been pulled for 3 or more seconds, stop & reset the timer and manually reload.
    if (intervalTimer_getTotalDurationInSeconds(RELOAD_TRIGGER_TIMER) >= RELOAD_TRIGGER_LENGTH_S) {
      reload();
    }
  }

  // End game loop...
  interrupts_disableArmInts();           // Done with game loop, disable the interrupts.
  hitLedTimer_turnLedOff();              // Save power :-)
  runningModes_printRunTimeStatistics(); // Print the run-time statistics.
}
