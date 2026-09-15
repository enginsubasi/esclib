#ifndef FIR_H_
#define FIR_H_

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

/* TYPEDEFS */

/* STRUCTURES */

typedef struct
{
    const float* taps;
    float* history;
    uint32_t length;
    uint32_t index;
    float output;
} fir_t;

typedef struct
{
    const int32_t* taps;
    int32_t* history;
    uint32_t length;
    uint32_t index;
    int32_t output;
} firi32_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t firInit ( fir_t* driver, const float* const taps, float* history, uint32_t length, float inputInit );
void firIteration ( fir_t* driver, float newData );
float firGetOutput ( const fir_t* const driver );

uint8_t firIniti32 ( firi32_t* driver, const int32_t* const taps, int32_t* history, uint32_t length, int32_t inputInit );
void firIterationi32 ( firi32_t* driver, int32_t newData );
int32_t firGetOutputi32 ( const firi32_t* const driver );

#ifdef __cplusplus
}
#endif

#endif /* FIR_H_ */
