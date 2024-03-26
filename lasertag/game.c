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

#include "hitLedTimer.h"
#include "interrupts.h"
#include "runningModes.h"
#include "sound/sound.h"
#include "switches.h"
#include "filter.h"

#define STARTING_LIVES 3
#define STARTING_BULLETS 10
#define HITS_PER_LIFE 5
#define TEAM_A_FREQUENCY 6
#define TEAM_B_FREQUENCY 9

#define re

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
  uint8_t bulletsLeft = STARTING_BULLETS;

    The player may force a reload of the clip at any time by pulling and holding the trigger for 3 seconds (if the clip contains shots, the initial push of the trigger will fire a shot).
    Immediately after losing a life, a player is invincible for 5 seconds thus giving them time to quickly scurry away. Note that you are not allowed to shoot while invincible.

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

  trigger_enable();                   // Makes the state machine responsive to the trigger.
  interrupts_enableTimerGlobalInts(); // Allow timer interrupts.
  interrupts_startArmPrivateTimer();  // Start the private ARM timer running.
  intervalTimer_reset(
      ISR_CUMULATIVE_TIMER); // Used to measure ISR execution time.
  intervalTimer_reset(
      TOTAL_RUNTIME_TIMER); // Used to measure total program execution time.
  intervalTimer_reset(
      MAIN_CUMULATIVE_TIMER); // Used to measure main-loop execution time.
  intervalTimer_start(
      TOTAL_RUNTIME_TIMER);   // Start measuring total execution time.
  interrupts_enableArmInts(); // ARM will now see interrupts after this.
  lockoutTimer_start();       // Ignore erroneous hits at startup (when all power
                              // values are essentially 0).

  // Implement game loop...
  while ((!(buttons_read() & BUTTONS_BTN3_MASK)) &&
         hitCount < MAX_HIT_COUNT) { // Run until you detect BTN3 pressed.
    transmitter_setFrequencyNumber(
        runningModes_getFrequencySetting());    // Read the switches and switch
                                                // frequency as required.
    intervalTimer_start(MAIN_CUMULATIVE_TIMER); // Measure run-time when you are
                                                // doing something.
    // Run filters, compute power, run hit-detection.
    detector(INTERRUPTS_CURRENTLY_ENABLED); // Interrupts are currently enabled.
    if (detector_hitDetected()) {           // Hit detected
      hitCount++;                           // increment the hit count.
      detector_clearHit();                  // Clear the hit.
      detector_hitCount_t
          hitCounts[DETECTOR_HIT_ARRAY_SIZE]; // Store the hit-counts here.
      detector_getHitCounts(hitCounts);       // Get the current hit counts.
      histogram_plotUserHits(hitCounts);      // Plot the hit counts on the TFT.
    }
    intervalTimer_stop(
        MAIN_CUMULATIVE_TIMER); // All done with actual processing.
  }

  // End game loop...
  interrupts_disableArmInts(); // Done with game loop, disable the interrupts.
  hitLedTimer_turnLedOff();    // Save power :-)
  runningModes_printRunTimeStatistics(); // Print the run-time statistics.
}

