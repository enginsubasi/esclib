#ifndef Q16_H_
#define Q16_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>

/* FUNCTION DEFINITIONS */

/* DEFINITIONS */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* The Q16 scale. A value of 1.0 is Q16_ONE. */
#define Q16_SHIFT   16
#define Q16_ONE     65536

/* TYPEDEFS */

/* STRUCTURES */

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

int32_t q16FromInt ( int32_t value );
int32_t q16ToInt ( int32_t value );
int32_t q16Mul ( int32_t multiplicand, int32_t multiplier );
int32_t q16Div ( int32_t dividend, int32_t divisor );
int32_t q16Sqrt ( int32_t value );

#ifdef __cplusplus
}
#endif

#endif /* Q16_H_ */
