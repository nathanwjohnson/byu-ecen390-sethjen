#include "detector.h"
#include "buffer.h"
#include "filter.h"
#include "lockoutTimer.h"
#include "hitLedTimer.h"

typedef uint16_t detector_hitCount_t; //TODO remove

static uint32_t fudgeFactor = 1000;

static bool detector_hitDetectedFlag = false;
static bool detector_ignoreAllHitsFlag = false;

static uint32_t invocation_count;
static uint32_t sample_cnt;
static uint16_t detector_hitArray[FILTER_FREQUENCY_COUNT];

// Initialize the detector module.
// By default, all frequencies are considered for hits.
// Assumes the filter module is initialized previously.
void detector_init(void) {

}

// freqArray is indexed by frequency number. If an element is set to true,
// the frequency will be ignored. Multiple frequencies can be ignored.
// Your shot frequency (based on the switches) is a good choice to ignore.
void detector_setIgnoredFrequencies(bool freqArray[]) {

}

bool detector_detectHit(double powerValues[]) {
    // sort power values
    uint8_t i, j;

    for (i = 1; i < FILTER_FREQUENCY_COUNT; i++) {
        double key = powerValues[i];
        j = i - 1;
 
        /* Move elements of arr[0..i-1], 
           that are greater than key, 
           to one position ahead of 
           their current position */
        while (j >= 0 && powerValues[j] > key) 
        {
            powerValues[j + 1] = powerValues[j];
            j = j - 1;
        }
        powerValues[j + 1] = key;
    }

    // select the median value
    double medianValue = powerValues[(FILTER_FREQUENCY_COUNT / 2 - 1)];

    // if the highest power filter is above the median value * fudge factor, return true
    return (powerValues[FILTER_FREQUENCY_COUNT] > (medianValue * fudgeFactor));
}

// Runs the entire detector: decimating FIR-filter, IIR-filters,
// power-computation, hit-detection. If interruptsCurrentlyEnabled = true,
// interrupts are running. If interruptsCurrentlyEnabled = false you can pop
// values from the ADC buffer without disabling interrupts. If
// interruptsCurrentlyEnabled = true, do the following:
// 1. disable interrupts.
// 2. pop the value from the ADC buffer.
// 3. re-enable interrupts.
// Ignore hits on frequencies specified with detector_setIgnoredFrequencies().
// Assumption: draining the ADC buffer occurs faster than it can fill.
void detector(bool interruptsCurrentlyEnabled) {
    invocation_count++;
    uint64_t elementCount = buffer_elements();

    for (int i = 0; i < elementCount; i++) {
        uint32_t rawAdcValue;
        if (interruptsCurrentlyEnabled) {
            interrupts_disableArmInts();
            rawAdcValue = buffer_pop();
            interrupts_enableArmInts();
        } else {
            rawAdcValue = buffer_pop();
        }

        // Scaling the ADC value to a double between -1.0 and 1.0
        double scaledAdcValue = ((double)rawAdcValue / 4095.0) * 2.0 - 1.0;

        filter_addNewInput(scaledAdcValue);

        sample_cnt++; // Count samples since last filter run
 
        // Run filters and hit detection if decimation factor reached
        if (sample_cnt == FILTER_FIR_DECIMATION_FACTOR) {
            uint16_t filterNumber;
            sample_cnt = 0; // Reset the sample count.
            filter_firFilter(); // Runs the FIR filter, output goes in the y-queue.
            // Run all the IIR filters and compute power in each of the output queues.
            for (filterNumber = 0; filterNumber < FILTER_FREQUENCY_COUNT; filterNumber++) {
                filter_iirFilter(filterNumber); // Run each of the IIR filters.
                // Compute the power for each of the filters, at lowest computational cost.
                // 1st false means do not compute from scratch.
                // 2nd false means no debug prints.
                filter_computePower(filterNumber, false, false);
            }
            if (!lockoutTimer_running()) {
                double powerValues[FILTER_FREQUENCY_COUNT];
                filter_getCurrentPowerValues(powerValues);
                uint8_t player_hit = detector_detectHit(powerValues);
                if (/*player hit was valid player*/ true) { //TODO fix this
                    lockoutTimer_start();
                    hitLedTimer_start();
                    detector_hitArray[player_hit]++;
                    detector_hitDetectedFlag = true;
                }
            }
        }
    }
}

// Returns true if a hit was detected.
bool detector_hitDetected(void) {
    return detector_hitDetectedFlag;
}

// Returns the frequency number that caused the hit.
uint16_t detector_getFrequencyNumberOfLastHit(void) {

}

// Clear the detected hit once you have accounted for it.
void detector_clearHit(void) {
    detector_hitDetectedFlag = false;
}

// Ignore all hits. Used to provide some limited invincibility in some game
// modes. The detector will ignore all hits if the flag is true, otherwise will
// respond to hits normally.
void detector_ignoreAllHits(bool flagValue) {
    detector_ignoreAllHitsFlag = flagValue;
}

// Get the current hit counts.
// Copy the current hit counts into the user-provided hitArray
// using a for-loop.
void detector_getHitCounts(detector_hitCount_t hitArray[]) {

}

// Allows the fudge-factor index to be set externally from the detector.
// The actual values for fudge-factors is stored in an array found in detector.c
void detector_setFudgeFactorIndex(uint32_t factorIdx) {

}

// Returns the detector invocation count.
// The count is incremented each time detector is called.
// Used for run-time statistics.
uint32_t detector_getInvocationCount(void) {
    return invocation_count;
}

/******************************************************
******************** Test Routines ********************
******************************************************/

// Students implement this as part of Milestone 3, Task 3.
// Create two sets of power values and call your hit detection algorithm
// on each set. With the same fudge factor, your hit detect algorithm
// should detect a hit on the first set and not detect a hit on the second.
void detector_runTest(void) {

}
