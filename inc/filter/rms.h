#ifndef RMS_H_
#define RMS_H_

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

/* The longest block the integer variant accepts. Its sums are sized for this
   many samples of the full int16_t range, and rms.c shows the arithmetic. */
#define RMS_I16_BLOCK_MAX   65536u

/* TYPEDEFS */

/* STRUCTURES */

typedef struct
{
    uint32_t blockLength;
    uint32_t count;

    float offset;
    float sum;
    float sumSquares;

    float mean;
    float rms;
    float acRms;

    uint8_t ready;
} rms_t;

typedef struct
{
    uint32_t blockLength;
    uint32_t count;

    int64_t sum;
    uint64_t sumSquares;

    int32_t mean;
    uint32_t rms;
    uint32_t acRms;

    uint8_t ready;
} rmsi16_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t rmsInit ( rms_t* driver, uint32_t blockLength );
void rmsIteration ( rms_t* driver, float sample );
uint8_t rmsIsReady ( rms_t* driver );
float rmsGetMean ( const rms_t* const driver );
float rmsGetRms ( const rms_t* const driver );
float rmsGetAcRms ( const rms_t* const driver );

uint8_t rmsIniti16 ( rmsi16_t* driver, uint32_t blockLength );
void rmsIterationi16 ( rmsi16_t* driver, int16_t sample );
uint8_t rmsIsReadyi16 ( rmsi16_t* driver );
int32_t rmsGetMeani16 ( const rmsi16_t* const driver );
uint32_t rmsGetRmsi16 ( const rmsi16_t* const driver );
uint32_t rmsGetAcRmsi16 ( const rmsi16_t* const driver );

#ifdef __cplusplus
}
#endif

#endif /* RMS_H_ */
