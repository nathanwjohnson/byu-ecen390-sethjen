/*
This software is provided for student assignment use in the Department of
Electrical and Computer Engineering, Brigham Young University, Utah, USA.
Users agree to not re-host, or redistribute the software, in source or binary
form, to other persons or other institutions. Users may modify and use the
source code for personal or educational use.
For questions, contact Brad Hutchings or Jeff Goeders, https://ece.byu.edu/
*/

#ifndef INVINCIBILITYTIMER_H_
#define INVINCIBILITYTIMER_H_

#include <stdbool.h>
#include <stdint.h>

// Perform any necessary inits for the invincibility timer.
void invincibilityTimer_init();

// Standard tick function.
void invincibilityTimer_tick();

// Calling this starts the timer.
void invincibilityTimer_start(uint32_t seconds);

// Returns true if the timer is running.
bool invincibilityTimer_running();

#endif /* INVINCIBILITYTIMER_H_ */
