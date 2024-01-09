/*
This software is provided for student assignment use in the Department of
Electrical and Computer Engineering, Brigham Young University, Utah, USA.
Users agree to not re-host, or redistribute the software, in source or binary
form, to other persons or other institutions. Users may modify and use the
source code for personal or educational use.
For questions, contact Brad Hutchings or Jeff Goeders, https://ece.byu.edu/
*/

#ifndef SOUND_H_
#define SOUND_H_

#include <stdbool.h>
#include <stdint.h>

typedef uint32_t sound_status_t;
#define SOUND_STATUS_OK 0
#define SOUND_STATUS_FAIL 1

// Sound levels.
#define SOUND_VOLUME_0 (INT16_MAX / 64) // Min volume.
#define SOUND_VOLUME_1 (INT16_MAX / 32)
#define SOUND_VOLUME_2 (INT16_MAX / 8)
#define SOUND_VOLUME_3 (INT16_MAX) // Max volume

// sound-specific defines.
typedef enum {
  sound_gameStart_e,       // Play a sound when the game starts.
  sound_gunFire_e,         // Standard laser firing sound.
  sound_hit_e,             // Player was hit by someone else.
  sound_gunClick_e,        // Player pulled trigger but the clip is empty.
  sound_gunReload_e,       // Sound made when the gun reloads.
  sound_loseLife_e,        // Sound made when you are hit enough times.
  sound_gameOver_e,        // Sound made when the game is over.
  sound_returnToBase_e,    // Remind the user that the game is over.
  sound_oneSecondSilence_e // One second of silence.
} sound_sounds_t;

// Just provide 4 volume settings.
// sound_lowVolume_e will be the default.
typedef enum {
  sound_minimumVolume_e = SOUND_VOLUME_0,    // Lowest setting.
  sound_mediumLowVolume_e = SOUND_VOLUME_1,  // Next loudest.
  sound_mediumHighVolume_e = SOUND_VOLUME_2, // Louder still.
  sound_maximumVolume_e = SOUND_VOLUME_3     // Really loud.
} sound_volume_t;

// Must be called before using the sound state machine.
sound_status_t sound_init();

// Standard tick function.
void sound_tick();

// Sets the sound and starts playing it immediately.
void sound_playSound(sound_sounds_t sound);

// Returns true if the sound is still playing.
bool sound_isBusy();

// Returns true if the sound has finished playing.
bool sound_isSoundComplete();

// Use this to set the base address for the array containing sound data.
// Allow sounds to be interrupted.
void sound_setSound(sound_sounds_t sound);

// Used to set the volume. Use one of the provided values.
void sound_setVolume(sound_volume_t);

// Tell the state machine to start playing the sound.
void sound_startSound();

// Stops playing the sound and resets the state-machine to the wait state.
void sound_stopSound();

// Plays several sounds.
// To invoke, just place this in your main.
// Completely stand alone, doesn't require interrupts, etc.
void sound_runTest();

#endif /* SOUND_H_ */