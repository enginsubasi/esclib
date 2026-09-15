#ifndef CORDIC_H_
#define CORDIC_H_

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

/* The Q16 scale of the sine and the cosine. An amplitude of 1.0 is
   CORDIC_ONE, so the results run from -CORDIC_ONE to +CORDIC_ONE. */
#define CORDIC_ONE              65536

/* The angle is a binary angle: the whole turn is 2^32 counts held in a
   uint32_t, so it wraps by itself and no pi appears anywhere. These name the
   three points a caller would otherwise write out by hand. */
#define CORDIC_QUARTER          0x40000000u
#define CORDIC_HALF             0x80000000u
#define CORDIC_THREEQUARTER     0xC0000000u

/* TYPEDEFS */

/* STRUCTURES */

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

void cordicSinCos ( uint32_t angle, int32_t* sine, int32_t* cosine );
int32_t cordicSin ( uint32_t angle );
int32_t cordicCos ( uint32_t angle );
void cordicPolar ( int32_t x, int32_t y, int32_t* magnitude, uint32_t* angle );
uint32_t cordicAtan2 ( int32_t y, int32_t x );
int32_t cordicMagnitude ( int32_t x, int32_t y );
void cordicRotate ( int32_t* x, int32_t* y, uint32_t angle );

#ifdef __cplusplus
}
#endif

#endif /* CORDIC_H_ */
