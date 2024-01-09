/*
This software is provided for student assignment use in the Department of
Electrical and Computer Engineering, Brigham Young University, Utah, USA.
Users agree to not re-host, or redistribute the software, in source or binary
form, to other persons or other institutions. Users may modify and use the
source code for personal or educational use.
For questions, contact Brad Hutchings or Jeff Goeders, https://ece.byu.edu/
*/

#ifndef RUNNINGMODES_H_
#define RUNNINGMODES_H_

#include <stdint.h>

// Prints out various run-time statistics on the TFT display.
// Assumes the following:
// detected interrupts is retrieved with interrupts_isrInvocationCount(),
// interval_timer(0) is the cumulative run-time of the ISR,
// interval_timer(1) is the total run-time,
// interval_timer(2) is the time spent in main running the filters, updating the
// display, and so forth. No comments in the code, the print statements are
// self-explanatory.
void runningModes_printRunTimeStatistics(void);

// Group all of the inits together to reduce visual clutter.
void runningModes_initAll(void);

// Returns the current switch-setting
uint16_t runningModes_getFrequencySetting(void);

// This mode runs until BTN3 is pressed.
// When BTN3 is pressed, it exits and prints performance information to the TFT.
// Transmits continuously and displays the received power on the TFT.
// Transmit frequency is selected via the slide-switches.
void runningModes_continuous(void);

// This mode runs until BTN3 is pressed.
// When BTN3 is pressed, it exits and prints performance information to the TFT.
// Press BTN0 or the gun-trigger to shoot.
// Each shot is registered on the histogram on the TFT.
// Transmit frequency is selected via the slide-switches.
void runningModes_shooter(void);

// This mode simply dumps raw ADC values to the console.
// It can be used to determine if bipolar mode is working for the ADC.
// Will loop forever. Stop the program with an external reset or Ctl-C.
void runningModes_dumpRawAdcValues(void);

#endif /* RUNNINGMODES_H_ */
