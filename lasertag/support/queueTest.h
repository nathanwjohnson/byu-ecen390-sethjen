/*
This software is provided for student assignment use in the Department of
Electrical and Computer Engineering, Brigham Young University, Utah, USA.
Users agree to not re-host, or redistribute the software, in source or binary
form, to other persons or other institutions. Users may modify and use the
source code for personal or educational use.
For questions, contact Brad Hutchings or Jeff Goeders, https://ece.byu.edu/
*/

#ifndef QUEUETEST_H_
#define QUEUETEST_H_

#include <stdbool.h>
#include "queue.h"

// Prints the current contents of the queue. Handy for debugging.
// Prints out the contents of the queue in the order of oldest element
// first to newest element last.
void queue_print(queue_t *q);

// Performs a comprehensive test of all queue functions. Returns false if the
// test fails, true otherwise. Prints out a series of informational messages
// during the test.
bool queue_runTest(void);

#endif /* QUEUETEST_H_ */
