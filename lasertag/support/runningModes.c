/*
This software is provided for student assignment use in the Department of
Electrical and Computer Engineering, Brigham Young University, Utah, USA.
Users agree to not re-host, or redistribute the software, in source or binary
form, to other persons or other institutions. Users may modify and use the
source code for personal or educational use.
For questions, contact Brad Hutchings or Jeff Goeders, https://ece.byu.edu/
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "buttons.h"
#include "detector.h"
#include "display.h"
#include "filter.h"
#include "histogram.h"
#include "hitLedTimer.h"
#include "interrupts.h"
#include "intervalTimer.h"
#include "isr.h"
#include "lockoutTimer.h"
#include "runningModes.h"
#include "switches.h"
#include "transmitter.h"
#include "trigger.h"
#include "utils.h"
#include "xparameters.h"

// Uncomment this code so that the code in the various modes will
// ignore your own frequency. You still must properly implement
// the ability to ignore frequencies in detector.c
//#define IGNORE_OWN_FREQUENCY 1

#define MAX_HIT_COUNT 100000

#define MAX_BUFFER_SIZE 100 // Used for a generic message buffer.

#define DETECTOR_HIT_ARRAY_SIZE                                                \
  FILTER_FREQUENCY_COUNT // The array contains one location per user frequency.

#define HISTOGRAM_BAR_COUNT                                                    \
  FILTER_FREQUENCY_COUNT // As many histogram bars as user filter frequencies.

#define ISR_CUMULATIVE_TIMER INTERVAL_TIMER_TIMER_0 // Used by the ISR.
#define TOTAL_RUNTIME_TIMER                                                    \
  INTERVAL_TIMER_TIMER_1 // Used to compute total run-time.
#define MAIN_CUMULATIVE_TIMER                                                  \
  INTERVAL_TIMER_TIMER_2 // Used to compute cumulative run-time in main.

#define SYSTEM_TICKS_PER_HISTOGRAM_UPDATE                                      \
  30000 // Update the histogram about 3 times per second.

#define RUNNING_MODE_WARNING_TEXT_SIZE 2 // Upsize the text for visibility.
#define RUNNING_MODE_WARNING_TEXT_COLOR DISPLAY_RED // Red for more visibility.
#define RUNNING_MODE_NORMAL_TEXT_SIZE 1 // Normal size for reporting.
#define RUNNING_MODE_NORMAL_TEXT_COLOR DISPLAY_WHITE // White for reporting.
#define RUNNING_MODE_SCREEN_X_ORIGIN 0 // Origin for reporting text.
#define RUNNING_MODE_SCREEN_Y_ORIGIN 0 // Origin for reporting text.

// Detector should be invoked this often for good performance.
#define SUGGESTED_DETECTOR_INVOCATIONS_PER_SECOND 30000
// ADC queue should have no more than this number of unprocessed elements for
// good performance.
#define SUGGESTED_REMAINING_ELEMENT_COUNT 500

// Defined to make things more readable.
#define INTERRUPTS_CURRENTLY_ENABLED true
#define INTERRUPTS_CURRENTLY_DISABLE false

// Prints out various run-time statistics on the TFT display.
// Assumes the following:
// detected interrupts is retrieved with interrupts_isrInvocationCount(),
// interval_timer(0) is the cumulative run time of the ISR,
// interval_timer(1) is the total run time,
// interval_timer(2) is the time spent in main running the filters, updating the
// display, and so forth. No comments in the code, the print statements are
// self-explanatory.
void runningModes_printRunTimeStatistics(void) {
  char sprintfBuffer[MAX_BUFFER_SIZE]; // Generic message buffer.
  // Setup the screen.
  display_setTextSize(RUNNING_MODE_NORMAL_TEXT_SIZE);
  display_setTextColor(RUNNING_MODE_NORMAL_TEXT_COLOR);
  display_setCursor(RUNNING_MODE_SCREEN_X_ORIGIN, RUNNING_MODE_SCREEN_Y_ORIGIN);
  display_fillScreen(DISPLAY_BLACK);

  // Print out the ADC mode.
  if (interrupts_getAdcInputMode() == INTERRUPTS_ADC_UNIPOLAR_MODE) {
    display_print("ADC mode: unipolar\n\n");
  } else if (interrupts_getAdcInputMode() == INTERRUPTS_ADC_BIPOLAR_MODE) {
    display_print("ADC mode: bipolar\n\n");
  }

  // Print out the number of unprocessed elements in ADC buffer.
  display_print("Unprocessed elements in ADC buffer: ");
  uint32_t remainingElementCount = buffer_elements();
  display_printDecimalInt(remainingElementCount);
  display_print("\n\n");

  // Print out total running time in seconds.
  double runningSeconds = intervalTimer_getTotalDurationInSeconds(TOTAL_RUNTIME_TIMER);
  display_print("Measured run time in seconds: ");
  sprintf(sprintfBuffer, "%.2f", runningSeconds);
  display_print(sprintfBuffer);
  display_print("\n\n");

  // Print out cumulative time spent in timer ISR.
  double isrRunningSeconds =
      intervalTimer_getTotalDurationInSeconds(ISR_CUMULATIVE_TIMER);
  display_print("Cumulative run time in timer ISR: ");
  sprintf(sprintfBuffer, "%.2f", isrRunningSeconds);
  display_print(sprintfBuffer);
  display_print(" (");
  sprintf(sprintfBuffer, "%.2f", isrRunningSeconds / runningSeconds * 100);
  display_print(sprintfBuffer);
  display_print("%)\n\n");

  // Print out cumulative time spent in detector.
  double mainLoopRunningSeconds =
      intervalTimer_getTotalDurationInSeconds(MAIN_CUMULATIVE_TIMER);
  display_print("Cumulative run time in detector: ");
  sprintf(sprintfBuffer, "%.2f", mainLoopRunningSeconds);
  display_print(sprintfBuffer);
  sprintf(sprintfBuffer, "%.2f",
          mainLoopRunningSeconds / runningSeconds * 100);
  display_print(" (");
  display_print(sprintfBuffer);
  display_print("%)\n\n");

  // Print out total interrupt count.
  uint32_t interruptCount = interrupts_isrInvocationCount();
  display_print("Total interrupts: ");
  display_printDecimalInt(interruptCount);
  display_print("\n\n");

  // Print out detector invocation statistics.
  uint32_t detectorInvocationCount = detector_getInvocationCount();
  display_print("Detector invocation count: ");
  display_printDecimalInt(detectorInvocationCount);
  display_print("\n\n");

  display_print("Detector invocations per second: ");
  sprintf(sprintfBuffer, "%.0f", detectorInvocationCount / runningSeconds);
  display_print(sprintfBuffer);
  display_print("\n\n");

  // If the detector invocation rate is too low, inform the user.
  if (detectorInvocationCount / runningSeconds <
      SUGGESTED_DETECTOR_INVOCATIONS_PER_SECOND) {
    display_setTextColor(RUNNING_MODE_WARNING_TEXT_COLOR);
    display_setTextSize(RUNNING_MODE_WARNING_TEXT_SIZE);
    display_print("Detector should be called\nat least ");
    display_printDecimalInt(SUGGESTED_DETECTOR_INVOCATIONS_PER_SECOND);
    display_print(" times per\nsecond.\n\n");
  }

  // If the unprocessed element count is too high, inform the user.
  if (remainingElementCount >= SUGGESTED_REMAINING_ELEMENT_COUNT) {
    display_setTextColor(RUNNING_MODE_WARNING_TEXT_COLOR);
    display_setTextSize(RUNNING_MODE_WARNING_TEXT_SIZE);
    display_print("ADC buffer should contain\nless than ");
    display_printDecimalInt(SUGGESTED_REMAINING_ELEMENT_COUNT);
    display_print(" elements.\n\n");
  }
}

// Group all of the inits together to reduce visual clutter.
void runningModes_initAll(void) {
  // Assume mio, leds, buttons, switches, & display initialized previously
  histogram_init(HISTOGRAM_BAR_COUNT);
  filter_init();
  detector_init();
  // isr_init() should include calls to: transmitter, trigger,
  // hitLedTimer, lockoutTimer, sound, and buffer init
  isr_init();
  intervalTimer_initAll();
  // Init all interrupts (but does not enable the interrupts at the devices).
  // Call last
  interrupts_initAll(false); // A true argument enables error messages
}

// Returns the current switch-setting
uint16_t runningModes_getFrequencySetting(void) {
  uint16_t switchSetting = switches_read() & 0xF; // Bit-mask the results.
  // Provide a nice default if the slide switches are in error.
  if (!(switchSetting < FILTER_FREQUENCY_COUNT))
    return FILTER_FREQUENCY_COUNT - 1;
  else
    return switchSetting;
}

// This mode runs until BTN3 is pressed.
// When BTN3 is pressed, it exits and prints performance information to the TFT.
// Transmits continuously and displays the received power on the TFT.
// Transmit frequency is selected via the slide-switches.
void runningModes_continuous(void) {
  runningModes_initAll(); // All necessary inits are called here.

  bool ignoredFrequencies[FILTER_FREQUENCY_COUNT];
  // setup the ignore frequencies array so you don't ignore any frequency.
  for (uint16_t i = 0; i < FILTER_FREQUENCY_COUNT; i++)
    ignoredFrequencies[i] = false;
#ifdef IGNORE_OWN_FREQUENCY
  printf("Ignoring own frequency.\n");
  ignoredFrequencies[runningModes_getFrequencySetting()] = true;
#endif
  detector_setIgnoredFrequencies(ignoredFrequencies);

  uint16_t histogramSystemTicks =
      0; // Only update the histogram display every so many ticks.
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

  transmitter_setContinuousMode(true); // Run the transmitter continuously.
  transmitter_run();           // Start the transmitter.
  while (!(buttons_read() &
           BUTTONS_BTN3_MASK)) { // Run until you detect BTN3 pressed.
    transmitter_setFrequencyNumber(runningModes_getFrequencySetting());
    histogramSystemTicks++;    // Keep track of ticks so you know when to update
                               // the histogram.
    // Run filters, compute power, etc.
    intervalTimer_start(MAIN_CUMULATIVE_TIMER); // Measure run-time when you are
                                                // doing something.
    detector(INTERRUPTS_CURRENTLY_ENABLED); // Interrupts are currently enabled.
    intervalTimer_stop(MAIN_CUMULATIVE_TIMER);
    // If enough ticks have transpired, update the histogram.
    if (histogramSystemTicks >= SYSTEM_TICKS_PER_HISTOGRAM_UPDATE) {
      double powerValues[FILTER_FREQUENCY_COUNT]; // Copy the current power
                                                  // values to here.
      filter_getCurrentPowerValues(
          powerValues); // Copy the current power values.
      histogram_plotUserFrequencyPower(
          powerValues); // Plot the power values on the TFT.
      histogramSystemTicks =
          0; // Reset the tick count and wait for the next update time.
    }
  }
  interrupts_disableArmInts();           // Stop interrupts.
  hitLedTimer_turnLedOff();              // Save power :-)
  runningModes_printRunTimeStatistics(); // Print the run-time statistics.
  printf("Continuous mode terminated.\n");
}

// This mode runs until BTN3 is pressed.
// When BTN3 is pressed, it exits and prints performance information to the TFT.
// Press BTN0 or the gun-trigger to shoot.
// Each shot is registered on the histogram on the TFT.
// Transmit frequency is selected via the slide-switches.
void runningModes_shooter(void) {
  uint16_t hitCount = 0;
  runningModes_initAll();

  // Init the ignored-frequencies so no frequencies are ignored.
  bool ignoredFrequencies[FILTER_FREQUENCY_COUNT];
  for (uint16_t i = 0; i < FILTER_FREQUENCY_COUNT; i++)
    ignoredFrequencies[i] = false;
#ifdef IGNORE_OWN_FREQUENCY
  printf("Ignoring own frequency.\n");
  ignoredFrequencies[runningModes_getFrequencySetting()] = true;
#endif
  detector_setIgnoredFrequencies(ignoredFrequencies);

  trigger_enable(); // Makes the state machine responsive to the trigger.
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
  lockoutTimer_start(); // Ignore erroneous hits at startup (when all power
                        // values are essentially 0).

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
  interrupts_disableArmInts(); // Done with loop, disable the interrupts.
  hitLedTimer_turnLedOff();    // Save power :-)
  runningModes_printRunTimeStatistics(); // Print the run-time statistics.
  printf("Shooter mode terminated after detecting %d hits.\n", hitCount);
}

// This mode simply dumps raw ADC values to the console.
// It can be used to determine if bipolar mode is working for the ADC.
// Will loop forever. Stop the program with an external reset or Ctl-C.
void runningModes_dumpRawAdcValues(void) {
  runningModes_initAll();

  // We don't need interrupts, so just loop and print out raw ADC values.
  while (1) {
    // In bipolar mode, the ADC returns a 12-bit signed value.
    // As such, you will need to do your own sign-extension out to 16 bits
    // in order to get to a C primitive type.
    // Thus, if bit-11 is a 1, bits 15-12 must also be set to 1, and
    // if bit-11 is a 0, bits 15-12 must also be set to a 0.
    int16_t signExtendedValue = interrupts_getAdcData();
    signExtendedValue |= (signExtendedValue & 0x800) ? 0xF000 : 0x0000;
    printf("raw ADC value: %d\n", signExtendedValue);
  }
}
