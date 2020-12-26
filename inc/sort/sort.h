#ifndef SORT_H_
#define SORT_H_

#include <stdint.h>

/* FUNCTION DEFINITIONS */

/* DEFINITIONS */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* TYPEDEFS */

/* STRUCTURES */

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

void selectionSort ( float* array, uint32_t length );
void bubbleSort ( float* array, uint32_t length );
void insertionSort ( float* array, uint32_t length );

#endif /* SORT_H_ */
