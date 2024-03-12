#include "detector.h"
#include "buffer.h"
#include "interrupts.h"
#include "filter.h"
#include "lockoutTimer.h"
#include "hitLedTimer.h"
#include <stdio.h>

static uint32_t fudgeFactors[3] = {10, 100, 1000};
static uint8_t fudgeFactorIndex = 2;

static bool detector_hitDetectedFlag = false;
static bool detector_ignoreAllHitsFlag = false;

static uint32_t invocation_count;
static uint32_t sample_cnt;
static uint16_t frequencyNumberOfLastHit;
static uint16_t detector_hitArray[FILTER_FREQUENCY_COUNT];
static bool ignored_frequencyArray[FILTER_FREQUENCY_COUNT];

// Initialize the detector module.
// By default, all frequencies are considered for hits.
// Assumes the filter module is initialized previously.
void detector_init(void) {
    // Initialize hit array to zero
    for (int i = 0; i < FILTER_FREQUENCY_COUNT; ++i) {
        detector_hitArray[i] = 0;
    }

    // By default, no frequencies are ignored
    for (int i = 0; i < FILTER_FREQUENCY_COUNT; ++i) {
        ignored_frequencyArray[i] = false;
    }

    fudgeFactorIndex = 2;

    detector_hitDetectedFlag = false;
    detector_ignoreAllHitsFlag = false;

    invocation_count = 0;
    sample_cnt = 0;
    frequencyNumberOfLastHit = 0;
}

// freqArray is indexed by frequency number. If an element is set to true,
// the frequency will be ignored. Multiple frequencies can be ignored.
// Your shot frequency (based on the switches) is a good choice to ignore.
void detector_setIgnoredFrequencies(bool freqArray[]) {
    for (int i = 0; i < FILTER_FREQUENCY_COUNT; ++i) {
        ignored_frequencyArray[i] = freqArray[i];
    }
}

bool detector_detectHit(double powerValues[]) {
    // sort power values
    uint8_t i, j;
    double powerValuesCopy[FILTER_FREQUENCY_COUNT];

    // copy power values to a new array so we don't alter the original
    for (int i = 0; i < FILTER_FREQUENCY_COUNT; ++i) {
        powerValuesCopy[i] = powerValues[i];
    }

    // iterate through each player frequencies
    for (uint8_t i = 0; i < FILTER_FREQUENCY_COUNT - 1; i++) {
        // compare each other frequency to the first one to know which ones to switch to get them in the right order
        for (uint8_t j = i + 1; j < FILTER_FREQUENCY_COUNT; j++) {
            // if the first value is less than the second, flip them
            if (powerValuesCopy[i] < powerValuesCopy[j]) {
                double temp = powerValuesCopy[i];
                powerValuesCopy[i] = powerValuesCopy[j];
                powerValuesCopy[j] = temp;
            }
        }
    }

    // select the median value
    double medianValue = powerValuesCopy[(FILTER_FREQUENCY_COUNT / 2 - 1)];

    // if the highest power filter is above the median value * fudge factor, return true
    return (powerValuesCopy[0] > (medianValue * fudgeFactors[fudgeFactorIndex]));
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

    // iterate through all new ADC values
    for (int i = 0; i < elementCount; i++) {

        uint32_t rawAdcValue;
        // if interrupts are enabled, we need to temporarily disable them to pop from the buffer
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
        if (sample_cnt >= FILTER_FIR_DECIMATION_FACTOR) {
            sample_cnt = 0; // Reset the sample count.
            filter_firFilter(); // Runs the FIR filter, output goes in the y-queue.
            // Run all the IIR filters and compute power in each of the output queues.
            for (uint16_t filterNumber = 0; filterNumber < FILTER_FREQUENCY_COUNT; filterNumber++) {
                filter_iirFilter(filterNumber); // Run each of the IIR filters.
                // Compute the power for each of the filters, at lowest computational cost.
                // 1st false means do not compute from scratch.
                // 2nd false means no debug prints.
                filter_computePower(filterNumber, false, false);
            }
            // can't be hit by other players if we are locked out
            if (!lockoutTimer_running()) {
                double powerValues[FILTER_FREQUENCY_COUNT];
                filter_getCurrentPowerValues(powerValues);

                // determine if this is a valid player hit and record it as such if it is
                if ((detector_detectHit(powerValues)) && (!detector_ignoreAllHitsFlag)) {
                    uint8_t player_hit = 2;
                    // find highest power player
                    for (uint8_t i = 0; i < FILTER_FREQUENCY_COUNT; i++) {
                        if (powerValues[i] > powerValues[player_hit])
                            player_hit = i;
                    }

                    if (!ignored_frequencyArray[player_hit]) {
                        lockoutTimer_start();
                        hitLedTimer_start();
                        detector_hitArray[player_hit]++;
                        detector_hitDetectedFlag = true;
                        frequencyNumberOfLastHit = player_hit;
                    }
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
    return frequencyNumberOfLastHit;
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
    for (int i = 0; i < FILTER_FREQUENCY_COUNT; ++i) {
        hitArray[i] = detector_hitArray[i];
    }
}

// Allows the fudge-factor index to be set externally from the detector.
// The actual values for fudge-factors is stored in an array found in detector.c
void detector_setFudgeFactorIndex(uint32_t factorIdx) {
    fudgeFactorIndex = factorIdx;
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
    filter_setCurrentPowerValue(0, 1050);
    filter_setCurrentPowerValue(1, 20);
    filter_setCurrentPowerValue(2, 40);
    filter_setCurrentPowerValue(3, 10);
    filter_setCurrentPowerValue(4, 15);
    filter_setCurrentPowerValue(5, 30);
    filter_setCurrentPowerValue(6, 35);
    filter_setCurrentPowerValue(7, 15);
    filter_setCurrentPowerValue(8, 25);
    filter_setCurrentPowerValue(9, 80);

    detector_setFudgeFactorIndex(0);

    double powerValues[FILTER_FREQUENCY_COUNT] = {0};
    filter_getCurrentPowerValues(powerValues);
    bool hitDetected = detector_detectHit(powerValues);

    printf("hit detected 1: %d\n", hitDetected ? 1 : 0);

    filter_setCurrentPowerValue(0, 33);
    filter_setCurrentPowerValue(1, 22);
    filter_setCurrentPowerValue(2, 42);
    filter_setCurrentPowerValue(3, 14);
    filter_setCurrentPowerValue(4, 17);
    filter_setCurrentPowerValue(5, 24);
    filter_setCurrentPowerValue(6, 14);
    filter_setCurrentPowerValue(7, 16);
    filter_setCurrentPowerValue(8, 40);
    filter_setCurrentPowerValue(9, 38);

    filter_getCurrentPowerValues(powerValues);
    hitDetected = detector_detectHit(powerValues);

    printf("hit detected 2: %d\n", hitDetected ? 1 : 0);
}
