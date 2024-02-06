#include "filter.h"
#include "queue.h"
#include <stdio.h>
#include <math.h>

#define FIR_COEFF_COUNT 81

#define FILTER_FREQUENCY_COUNT 10

#define IIR_COEFF_COUNT 11

#define X_QUEUE_SIZE FIR_COEFF_COUNT
#define Y_QUEUE_SIZE IIR_COEFF_COUNT
#define Z_QUEUE_SIZE (IIR_COEFF_COUNT - 1)
#define OUTPUT_QUEUE_SIZE FILTER_INPUT_PULSE_WIDTH

#define QUEUE_INIT_VALUE 0

const static double fir_b_coeffs[FIR_COEFF_COUNT] = {
    6.2534348595847538e-04, 6.5497758294040542e-04, 6.1992178501587701e-04, 5.0452526771031455e-04, 2.9091060249592421e-04, -3.2856141914564076e-05, -4.6270378618655110e-04, -9.6927546688259272e-04, -1.4924081755106418e-03, -1.9419900366783919e-03, -2.2067863671876870e-03, -2.1712756177317168e-03, -1.7387264211219384e-03, -8.5702012741646952e-04, 4.5755838533190820e-04, 2.1038619889547699e-03, 3.8916195777932861e-03, 5.5528025909850429e-03, 6.7697616171742822e-03, 7.2184438752610595e-03, 6.6220735304987509e-03, 4.8081553873736364e-03, 1.7600311340430473e-03, -2.3461497646870785e-03, -7.1270921927757249e-03, -1.2006185309970628e-02, -1.6257372605455990e-02, -1.9076605069723938e-02, -1.9674051143141542e-02, -1.7376505856439812e-02, -1.1726971420919888e-02, -2.5676376647722600e-03, 9.9063015042762728e-03, 2.5131461770900417e-02, 4.2204543223913080e-02, 5.9953325291499965e-02, 7.7043897907315209e-02, 9.2112551316003752e-02, 1.0390705353179479e-01, 1.1142031823958311e-01, 1.1400000000000000e-01, 1.1142031823958311e-01, 1.0390705353179479e-01, 9.2112551316003752e-02, 7.7043897907315209e-02, 5.9953325291499965e-02, 4.2204543223913080e-02, 2.5131461770900417e-02, 9.9063015042762728e-03, -2.5676376647722600e-03, -1.1726971420919888e-02, -1.7376505856439812e-02, -1.9674051143141542e-02, -1.9076605069723938e-02, -1.6257372605455990e-02, -1.2006185309970628e-02, -7.1270921927757249e-03, -2.3461497646870785e-03, 1.7600311340430473e-03, 4.8081553873736364e-03, 6.6220735304987509e-03, 7.2184438752610595e-03, 6.7697616171742822e-03, 5.5528025909850429e-03, 3.8916195777932861e-03, 2.1038619889547699e-03, 4.5755838533190820e-04, -8.5702012741646952e-04, -1.7387264211219384e-03, -2.1712756177317168e-03, -2.2067863671876870e-03, -1.9419900366783919e-03, -1.4924081755106418e-03, -9.6927546688259272e-04, -4.6270378618655110e-04, -3.2856141914564076e-05, 2.9091060249592421e-04, 5.0452526771031455e-04, 6.1992178501587701e-04, 6.5497758294040542e-04, 6.2534348595847538e-04
};

const static double iir_a_coeffs[FILTER_FREQUENCY_COUNT][IIR_COEFF_COUNT] = {
    {1.0000000000000000e+00, -5.9637727070164059e+00, 1.9125339333078287e+01, -4.0341474540744301e+01, 6.1537466875369077e+01, -7.0019717951472558e+01, 6.0298814235239249e+01, -3.8733792862566574e+01, 1.7993533279581207e+01, -5.4979061224868158e+00, 9.0332828533800469e-01},
    {1.0000000000000000e+00, -4.6377947119071408e+00, 1.3502215749461552e+01, -2.6155952405269698e+01, 3.8589668330738235e+01, -4.3038990303252490e+01, 3.7812927599536991e+01, -2.5113598088113683e+01, 1.2703182701888030e+01, -4.2755083391143280e+00, 9.0332828533799747e-01},
    {1.0000000000000000e+00, -3.0591317915750937e+00, 8.6417489609637492e+00, -1.4278790253808838e+01, 2.1302268283304294e+01, -2.2193853972079211e+01, 2.0873499791105424e+01, -1.3709764520609379e+01, 8.1303553577931567e+00, -2.8201643879900473e+00, 9.0332828533799880e-01},
    {1.0000000000000000e+00, -1.4071749185996751e+00, 5.6904141470697542e+00, -5.7374718273676306e+00, 1.1958028362868905e+01, -8.5435280598354630e+00, 1.1717345583835968e+01, -5.5088290876998647e+00, 5.3536787286077674e+00, -1.2972519209655595e+00, 9.0332828533800047e-01},
    {1.0000000000000000e+00, 8.2010906117760318e-01, 5.1673756579268604e+00, 3.2580350909220925e+00, 1.0392903763919193e+01, 4.8101776408669084e+00, 1.0183724507092508e+01, 3.1282000712126754e+00, 4.8615933365571991e+00, 7.5604535083144919e-01, 9.0332828533800047e-01},
    {1.0000000000000000e+00, 2.7080869856154530e+00, 7.8319071217995795e+00, 1.2201607990980769e+01, 1.8651500443681677e+01, 1.8758157568004620e+01, 1.8276088095999114e+01, 1.1715361303018966e+01, 7.3684394621254015e+00, 2.4965418284512091e+00, 9.0332828533801224e-01},
    {1.0000000000000000e+00, 4.9479835250075892e+00, 1.4691607003177602e+01, 2.9082414772101060e+01, 4.3179839108869331e+01, 4.8440791644688879e+01, 4.2310703962394342e+01, 2.7923434247706432e+01, 1.3822186510471010e+01, 4.5614664160654357e+00, 9.0332828533799958e-01},
    {1.0000000000000000e+00, 6.1701893352279864e+00, 2.0127225876810336e+01, 4.2974193398071691e+01, 6.5958045321253465e+01, 7.5230437667866624e+01, 6.4630411355739881e+01, 4.1261591079244141e+01, 1.8936128791950541e+01, 5.6881982915180327e+00, 9.0332828533799836e-01},
    {1.0000000000000000e+00, 7.4092912870072398e+00, 2.6857944460290135e+01, 6.1578787811202247e+01, 9.8258255839887340e+01, 1.1359460153696304e+02, 9.6280452143026153e+01, 5.9124742025776442e+01, 2.5268527576524235e+01, 6.8305064480743178e+00, 9.0332828533800158e-01},
    {1.0000000000000000e+00, 8.5743055776347692e+00, 3.4306584753117903e+01, 8.4035290411037124e+01, 1.3928510844056831e+02, 1.6305115418161643e+02, 1.3648147221895812e+02, 8.0686288623299902e+01, 3.2276361903872186e+01, 7.9045143816244918e+00, 9.0332828533799903e-01}};

const static double iir_b_coeffs[FILTER_FREQUENCY_COUNT][IIR_COEFF_COUNT] = {
    {9.0928661148176830e-10, 0.0, -4.5464330574088414e-09, 0.0, 9.0928661148176828e-09, 0.0, -9.0928661148176828e-09, 0.0, 4.5464330574088414e-09, 0.0, -9.0928661148176830e-10},
    {9.0928661148203093e-10, 0.0, -4.5464330574101550e-09, 0.0, 9.0928661148203099e-09, 0.0, -9.0928661148203099e-09, 0.0, 4.5464330574101550e-09, 0.0, -9.0928661148203093e-10},
    {9.0928661148196858e-10, 0.0, -4.5464330574098431e-09, 0.0, 9.0928661148196862e-09, 0.0, -9.0928661148196862e-09, 0.0, 4.5464330574098431e-09, 0.0, -9.0928661148196858e-10},
    {9.0928661148203424e-10, 0.0, -4.5464330574101715e-09, 0.0, 9.0928661148203430e-09, 0.0, -9.0928661148203430e-09, 0.0, 4.5464330574101715e-09, 0.0, -9.0928661148203424e-10},
    {9.0928661148203041e-10, 0.0, -4.5464330574101516e-09, 0.0, 9.0928661148203033e-09, 0.0, -9.0928661148203033e-09, 0.0, 4.5464330574101516e-09, 0.0, -9.0928661148203041e-10},
    {9.0928661148164309e-10, 0.0, -4.5464330574082152e-09, 0.0, 9.0928661148164304e-09, 0.0, -9.0928661148164304e-09, 0.0, 4.5464330574082152e-09, 0.0, -9.0928661148164309e-10},
    {9.0928661148193684e-10, 0.0, -4.5464330574096843e-09, 0.0, 9.0928661148193686e-09, 0.0, -9.0928661148193686e-09, 0.0, 4.5464330574096843e-09, 0.0, -9.0928661148193684e-10},
    {9.0928661148192133e-10, 0.0, -4.5464330574096065e-09, 0.0, 9.0928661148192131e-09, 0.0, -9.0928661148192131e-09, 0.0, 4.5464330574096065e-09, 0.0, -9.0928661148192133e-10},
    {9.0928661148181700e-10, 0.0, -4.5464330574090846e-09, 0.0, 9.0928661148181692e-09, 0.0, -9.0928661148181692e-09, 0.0, 4.5464330574090846e-09, 0.0, -9.0928661148181700e-10},
    {9.0928661148189248e-10, 0.0, -4.5464330574094626e-09, 0.0, 9.0928661148189252e-09, 0.0, -9.0928661148189252e-09, 0.0, 4.5464330574094626e-09, 0.0, -9.0928661148189248e-10}};

static queue_t xQueue;
static queue_t yQueue;
static queue_t zQueues[FILTER_FREQUENCY_COUNT];
static queue_t outputQueues[FILTER_FREQUENCY_COUNT];

static double currentPowerValue[FILTER_FREQUENCY_COUNT];
static double oldest_value[FILTER_FREQUENCY_COUNT];

/******************************************************************************
***** Helper functions
******************************************************************************/

// Call queue_init() on xQueue and fill it with zeros.
void initXQueue() {
    queue_init(&xQueue, X_QUEUE_SIZE, "xQueue");

    // Fill queue with zeros
    for (uint32_t i = 0; i < X_QUEUE_SIZE; i++) {
        queue_overwritePush(&xQueue, QUEUE_INIT_VALUE);
    }
}

// Call queue_init() on yQueue and fill it with zeros.
void initYQueue() {
    queue_init(&yQueue, Y_QUEUE_SIZE, "yQueue");

    // Fill queue with zeros
    for (uint32_t i = 0; i < Y_QUEUE_SIZE; i++) {
        queue_overwritePush(&yQueue, QUEUE_INIT_VALUE);
    }
}

// Call queue_init() on all of the zQueues and fill each z queue with zeros.
void initZQueues() {
    for (uint32_t i = 0; i < FILTER_FREQUENCY_COUNT; i++) {
        queue_init(&(zQueues[i]), Z_QUEUE_SIZE, "zQueue");

        for (uint32_t j = 0; j < Z_QUEUE_SIZE; j++) {
            queue_overwritePush(&(zQueues[i]), QUEUE_INIT_VALUE);
        }
    }
}

// Call queue_init() on all of the outputQueues and fill each outputQueue with zeros.
void initOutputQueues() {
    for (uint32_t i = 0; i < FILTER_FREQUENCY_COUNT; i++) {
        queue_init(&(outputQueues[i]), OUTPUT_QUEUE_SIZE, "outputQueue");

        for (uint32_t j = 0; j < OUTPUT_QUEUE_SIZE; j++) {
            queue_overwritePush(&(outputQueues[i]), QUEUE_INIT_VALUE);
        }
    }
}

// 1. First filter is a decimating FIR filter with a configurable number of taps
// and decimation factor.
// 2. The output from the decimating FIR filter is passed through a bank of 10
// IIR filters. The characteristics of the IIR filter are fixed.

/******************************************************************************
***** Main Filter Functions
******************************************************************************/

// Must call this prior to using any filter functions.
void filter_init()
{
  // Init queues and fill them with zeros.
  initXQueue();  // Call queue_init() on xQueue and fill it with zeros.
  initYQueue();  // Call queue_init() on yQueue and fill it with zeros.
  initZQueues(); // Call queue_init() on all of the zQueues and fill each z queue with zeros.
  initOutputQueues();  // Call queue_init() on all of the outputQueues and fill each outputQueue with zeros.
}

// Use this to copy an input into the input queue of the FIR-filter (xQueue).
void filter_addNewInput(double x)
{
    queue_overwritePush(&xQueue, x);
}

// Invokes the FIR-filter. Input is contents of xQueue.
// Output is returned and is also pushed on to yQueue.
double filter_firFilter()
{
    double y = 0.0;

    // This for-loop performs the identical computation to that shown above.
    for (uint32_t i=0; i < FIR_COEFF_COUNT; i++) { // iteratively adds the (b * input) products.
        y += queue_readElementAt(&xQueue, ((FIR_COEFF_COUNT - 1) - i)) * fir_b_coeffs[i];
    }

    queue_overwritePush(&yQueue, y);

    return y;
}

// Use this to invoke a single iir filter. Input comes from yQueue.
// Output is returned and is also pushed onto zQueue[filterNumber].
double filter_iirFilter(uint16_t filterNumber)
{
    double y = 0.0;
    double z = 0.0;

    for (uint32_t i = 0; i < Y_QUEUE_SIZE; i++) {
        y += queue_readElementAt(&yQueue, (Y_QUEUE_SIZE - i - 1)) * iir_b_coeffs[filterNumber][i];
    }

    for (uint32_t i = 0; i < Z_QUEUE_SIZE; i++) {
        z += queue_readElementAt(&(zQueues[filterNumber]), (Z_QUEUE_SIZE - i - 1)) * iir_a_coeffs[filterNumber][i];
    }

    z = y - z;

    queue_overwritePush(&(outputQueues[filterNumber]), z);
    queue_overwritePush(&(zQueues[filterNumber]), z);

    return z;
}


// Use this to compute the power for values contained in an outputQueue.
// If force == true, then recompute power by using all values in the
// outputQueue. This option is necessary so that you can correctly compute power
// values the first time. After that, you can incrementally compute power values
// by:
// 1. Keeping track of the power computed in a previous run, call this
// prev-power.
// 2. Keeping track of the oldest outputQueue value used in a previous run, call
// this oldest-output-value.
// 3. Get the newest value from the power queue, call this newest-power-value.
// 4. Compute new power as: prev-power - (oldest-value * oldest-value) +
// (newest-value * newest-value). Note that this function will probably need an
// array to keep track of these values for each of the 10 output queues.
double filter_computePower(uint16_t filterNumber, bool forceComputeFromScratch,
                           bool debugPrint)
{
    if (forceComputeFromScratch) {
        double power = 0;

        for (double i = 0; i < queue_elementCount(&outputQueues[filterNumber]); i++) {
            power += pow(queue_readElementAt(&outputQueues[filterNumber], i), 2);
        }

        currentPowerValue[filterNumber] = power;
    } else {
        currentPowerValue[filterNumber] = currentPowerValue[filterNumber] - 
            pow(oldest_value[filterNumber], 2) +
            pow(queue_readElementAt(&outputQueues[filterNumber], (queue_elementCount(&outputQueues[filterNumber]) - 1)), 2);
    }

    oldest_value[filterNumber] = queue_readElementAt(&outputQueues[filterNumber], 0); // Store the oldest output value for next loop

    return currentPowerValue[filterNumber];
}

// Returns the last-computed output power value for the IIR filter
// [filterNumber].
double filter_getCurrentPowerValue(uint16_t filterNumber)
{
    return currentPowerValue[filterNumber];
}

// Sets a current power value for a specific filter number.
// Useful in testing the detector.
void filter_setCurrentPowerValue(uint16_t filterNumber, double value)
{
  currentPowerValue[filterNumber] = value;
}

// Get a copy of the current power values.
// This function copies the already computed values into a previously-declared
// array so that they can be accessed from outside the filter software by the
// detector. Remember that when you pass an array into a C function, changes to
// the array within that function are reflected in the returned array.
void filter_getCurrentPowerValues(double powerValues[])
{
    // powerValues = currentPowerValue;
    for (uint16_t i = 0; i < FILTER_FREQUENCY_COUNT; ++i) {
        powerValues[i] = currentPowerValue[i];
    }
}

// Using the previously-computed power values that are currently stored in
// currentPowerValue[] array, copy these values into the normalizedArray[]
// argument and then normalize them by dividing all of the values in
// normalizedArray by the maximum power value contained in currentPowerValue[].
// The pointer argument indexOfMaxValue is used to return the index of the
// maximum value. If the maximum power is zero, make sure to not divide by zero
// and that *indexOfMaxValue is initialized to a sane value (like zero).
void filter_getNormalizedPowerValues(double normalizedArray[],
                                     uint16_t *indexOfMaxValue)
{
    double maxPower = 0.0;
    *indexOfMaxValue = 0;
    for (uint16_t i = 0; i < FILTER_FREQUENCY_COUNT; i++) {
        if (currentPowerValue[i] > maxPower) {
            maxPower = currentPowerValue[i];
            *indexOfMaxValue = i;
        }
    }
    
    if (maxPower > 0) {
        for (uint16_t i = 0; i < FILTER_FREQUENCY_COUNT; i++) {
            normalizedArray[i] = currentPowerValue[i] / maxPower;
        }
    } else {
        // If maxPower is zero, set all normalized values to zero
        for (uint16_t i = 0; i < FILTER_FREQUENCY_COUNT; i++) {
            normalizedArray[i] = 0.0;
        }
    }
}


/******************************************************************************
***** Verification-Assisting Functions
***** External test functions access the internal data structures of filter.c
***** via these functions. They are not used by the main filter functions.
******************************************************************************/

// Returns the array of FIR coefficients.
const double *filter_getFirCoefficientArray()
{
    return fir_b_coeffs;
}

// Returns the number of FIR coefficients.
uint32_t filter_getFirCoefficientCount()
{
    return FIR_COEFF_COUNT;
}

// Returns the array of a coefficients for a particular filter number.
const double *filter_getIirACoefficientArray(uint16_t filterNumber)
{
    return iir_a_coeffs[filterNumber];
}

// Returns the number of A coefficients.
uint32_t filter_getIirACoefficientCount()
{
    return FILTER_FIR_DECIMATION_FACTOR;
}

// Returns the array of b coefficients for a particular filter number.
const double *filter_getIirBCoefficientArray(uint16_t filterNumber)
{
    return iir_b_coeffs[filterNumber];
}

// Returns the number of B coefficients.
uint32_t filter_getIirBCoefficientCount()
{
    return FILTER_FIR_DECIMATION_FACTOR;
}

// Returns the size of the yQueue.
uint32_t filter_getYQueueSize()
{
    return Y_QUEUE_SIZE;
}

// Returns the decimation value.
uint16_t filter_getDecimationValue()
{
    return FILTER_FIR_DECIMATION_FACTOR;
}

// Returns the address of xQueue.
queue_t *filter_getXQueue()
{
    return &xQueue;
}

// Returns the address of yQueue.
queue_t *filter_getYQueue()
{
    return &yQueue;
}

// Returns the address of zQueue for a specific filter number.
queue_t *filter_getZQueue(uint16_t filterNumber)
{
    return &(zQueues[filterNumber]);
}

// Returns the address of the IIR output-queue for a specific filter-number.
queue_t *filter_getIirOutputQueue(uint16_t filterNumber)
{
    return &(outputQueues[filterNumber]);
}