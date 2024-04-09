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
#include "histogram.h"
#include "hitLedTimer.h"
#include "include/mio.h"
#include "interrupts.h"
#include "intervalTimer.h"
#include "lockoutTimer.h"
#include "runningModes.h"
#include "sound/sound.h"
#include "switches.h"
#include "transmitter.h"
#include "trigger.h"

#define STARTING_LIVES 3
#define STARTING_BULLETS 10
#define STARTING_HITS 5
#define HITS_PER_LIFE 5
#define HITS_TO_REVIVE 10
#define TEAM_A_FREQUENCY 6
#define TEAM_B_FREQUENCY 9
#define TRIGGER_GUN_TRIGGER_MIO_PIN 10
#define GUN_TRIGGER_PRESSED 1
#define INVINCIBILITY_TIME 5
#define REVIVE_TIME 10

#define DETECTOR_HIT_ARRAY_SIZE \
  FILTER_FREQUENCY_COUNT // The array contains one location per user frequency.

// Defined to make things more readable.
#define INTERRUPTS_CURRENTLY_ENABLED true
#define INTERRUPTS_CURRENTLY_DISABLE false

#define ISR_CUMULATIVE_TIMER INTERVAL_TIMER_TIMER_0 // Used by the ISR.
#define RELOAD_TRIGGER_TIMER \
  INTERVAL_TIMER_TIMER_1
#define INVINCIBILITY_AND_REVIVE_TIMER \
  INTERVAL_TIMER_TIMER_2
#define RELOAD_TRIGGER_LENGTH_S 3

volatile static uint16_t bulletsLeft = STARTING_BULLETS;
volatile static uint16_t livesLeft = STARTING_LIVES;
volatile static uint16_t hitsLeft = STARTING_HITS;

volatile static bool canRevive = true;
volatile static uint16_t reviveHits = HITS_TO_REVIVE;

// Trigger can be activated by either btn0 or the external gun that is attached to TRIGGER_GUN_TRIGGER_MIO_PIN
// Gun input is ignored if the gun-input is high when the init() function is invoked.
bool triggerIsPressed() {
  return (mio_readPin(TRIGGER_GUN_TRIGGER_MIO_PIN) == GUN_TRIGGER_PRESSED);
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
  sound_setVolume(sound_mediumHighVolume_e);
  sound_setSound(sound_gameStart_e);
  sound_startSound();
  intervalTimer_init(RELOAD_TRIGGER_TIMER);
  intervalTimer_init(INVINCIBILITY_AND_REVIVE_TIMER);

  // Configuration...

  livesLeft = STARTING_LIVES;
  bool gameOver = false;
  bool invincible = false;
  bool playerIsDead = false;

  bool reloadTriggerTimerRunning = false;
  bool autoReloadTimerRunning = false;

  bool ignoredFrequencies[FILTER_FREQUENCY_COUNT];
  for (uint16_t i = 0; i < FILTER_FREQUENCY_COUNT; i++)
    ignoredFrequencies[i] = true;

  // read switch 0 to determine team membership
  uint16_t switchSetting = switches_read() & SWITCHES_SW0_MASK; // Bit-mask the results.
  if (switchSetting) {                                          // Team B
    transmitter_setFrequencyNumber(TEAM_B_FREQUENCY);
    ignoredFrequencies[TEAM_A_FREQUENCY] = false;
    // ignoredFrequencies[TEAM_B_FREQUENCY] = false; // TODO remove, lets you shoot yourself
    printf("Team B\n");
  } else { // Team A
    transmitter_setFrequencyNumber(TEAM_A_FREQUENCY);
    ignoredFrequencies[TEAM_B_FREQUENCY] = false;
    // ignoredFrequencies[TEAM_A_FREQUENCY] = false; // TODO remove
    printf("Team A\n");
  }
  detector_setIgnoredFrequencies(ignoredFrequencies);

  interrupts_enableTimerGlobalInts(); // Allow timer interrupts.
  interrupts_startArmPrivateTimer();  // Start the private ARM timer running.
  interrupts_enableArmInts();         // ARM will now see interrupts after this.
  lockoutTimer_start();               // Ignore erroneous hits at startup (when all power
                                      // values are essentially 0).
  intervalTimer_reset(RELOAD_TRIGGER_TIMER);
  intervalTimer_reset(INVINCIBILITY_AND_REVIVE_TIMER);

  // clears the first hit on startup
  while (sound_isBusy()) {
    // Run filters, compute power, run hit-detection.
    detector(INTERRUPTS_CURRENTLY_ENABLED); // Interrupts are currently enabled.
    detector_clearHit();
  }

  trigger_enable(); // Makes the state machine responsive to the trigger.

  // Implement game loop...
  while (!gameOver) { // Run until you detect BTN3 pressed.
    // If the player is dead, play a death sound every few seconds
    if (playerIsDead) {
      if (intervalTimer_getTotalDurationInSeconds(INVINCIBILITY_AND_REVIVE_TIMER) >= REVIVE_TIME) {
        canRevive = false;
      }

      if (canRevive) {
        // If there is a hit detected, handle it
        if (detector_hitDetected()) { // Hit detected
          // check if it's from our team. If it's not, continue.

          // Only handle hits from your own team. If you've been hit HITS_TO_REVIVE times by your teammate, then revive.
          if (detector_getFrequencyNumberOfLastHit() == transmitter_getFrequencyNumber()) {
            reviveHits++;
            if (reviveHits >= HITS_TO_REVIVE) {
              playerIsDead = false;
              hitsLeft = STARTING_HITS;
              livesLeft = STARTING_LIVES;
              reviveHits = 0;
            }
          }

          detector_clearHit(); // Clear the hit.
        }
      } else {
        if (intervalTimer_getTotalDurationInSeconds(INVINCIBILITY_AND_REVIVE_TIMER) >= INVINCIBILITY_TIME) {
          if (!sound_isBusy()) {
            sound_setSound(sound_returnToBase_e);
            sound_startSound();
            intervalTimer_stop(INVINCIBILITY_AND_REVIVE_TIMER);
            intervalTimer_reset(INVINCIBILITY_AND_REVIVE_TIMER);
            intervalTimer_start(INVINCIBILITY_AND_REVIVE_TIMER);
          }
        }
      }
      continue;
    }

    // Handle invincibility
    if (invincible) {
      // If the invincibility timer is up, stop it and turn off invincibility
      if (intervalTimer_getTotalDurationInSeconds(INVINCIBILITY_AND_REVIVE_TIMER) >= INVINCIBILITY_TIME) {
        intervalTimer_stop(INVINCIBILITY_AND_REVIVE_TIMER);
        invincible = false;
        intervalTimer_reset(INVINCIBILITY_AND_REVIVE_TIMER);

        // not sure if is supposed to, but I have it reload the bullets after you lose a life
        trigger_setRemainingShotCount(STARTING_BULLETS);
        bulletsLeft = STARTING_BULLETS;
        // Run filters, compute power, run hit-detection.
        detector(INTERRUPTS_CURRENTLY_ENABLED); // Interrupts are currently enabled.
        detector_clearHit();
      }
      trigger_disable();
      continue;
    } else {
      trigger_enable();
    }

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

      hitsLeft--;
      printf("hits left: %d\n", hitsLeft);
      sound_setSound(sound_hit_e);
      sound_startSound();

      // if you have been hit five times, lose a life
      if (hitsLeft == 0) {
        hitsLeft = STARTING_HITS;
        livesLeft--;
        printf("lives left: %d\n", livesLeft);

        if (livesLeft == 0) {
          // you are now dead. Start revive timer.

          playerIsDead = true;

          if (!canRevive) {
            // You are dead and can't be revived. Game over!
            trigger_disable();
            sound_setSound(sound_gameOver_e);
            sound_startSound();
          }

          // If game over, use the invincibility and revive timer to time game over sound. Else, use it to time revive
          intervalTimer_start(INVINCIBILITY_AND_REVIVE_TIMER);
        } else { // start 5 second invincibility timer
          intervalTimer_start(INVINCIBILITY_AND_REVIVE_TIMER);
          invincible = true;

          // play losing a life sound
          sound_setSound(sound_loseLife_e);
          sound_startSound();
        }
      }
    }

    // If the trigger is pressed, start a timer
    if (triggerIsPressed()) {
      if (!reloadTriggerTimerRunning && !autoReloadTimerRunning) {
        intervalTimer_start(RELOAD_TRIGGER_TIMER);
        reloadTriggerTimerRunning = true;
      }
    } else if (reloadTriggerTimerRunning && !autoReloadTimerRunning) {
      // if the trigger isn't pressed but the trigger timer is still running, stop and reset the timer.
      intervalTimer_stop(RELOAD_TRIGGER_TIMER);
      reloadTriggerTimerRunning = false;
      intervalTimer_reset(RELOAD_TRIGGER_TIMER);
    }

    // if the trigger has been pulled for 3 or more seconds, stop & reset the timer and manually reload.
    if (intervalTimer_getTotalDurationInSeconds(RELOAD_TRIGGER_TIMER) >= RELOAD_TRIGGER_LENGTH_S) {
      printf("Reload\n");
      trigger_setRemainingShotCount(STARTING_BULLETS);
      bulletsLeft = STARTING_BULLETS;
      sound_setSound(sound_gunReload_e);
      sound_startSound();
      intervalTimer_stop(RELOAD_TRIGGER_TIMER);
      reloadTriggerTimerRunning = false;
      autoReloadTimerRunning = false;
      intervalTimer_reset(RELOAD_TRIGGER_TIMER);
    }

    // this will be true once for every bullet fired
    if (trigger_getRemainingShotCount() != bulletsLeft) {
      bulletsLeft--;
      // if gun is fired and is out of bullets, play click sound and wait 3 seconds to reload
      if (bulletsLeft > STARTING_BULLETS) {
        printf("gun empty\n");
        sound_setSound(sound_gunClick_e);
        sound_startSound();
        intervalTimer_start(RELOAD_TRIGGER_TIMER);
        autoReloadTimerRunning = true;
      } else { // play firing sound
        printf("shots left: %d\n", bulletsLeft);
        sound_setSound(sound_gunFire_e);
        sound_startSound();
      }
    }
  }

  // End game loop...
  interrupts_disableArmInts();           // Done with game loop, disable the interrupts.
  hitLedTimer_turnLedOff();              // Save power :-)
  runningModes_printRunTimeStatistics(); // Print the run-time statistics.
}
