#ifndef SOFTTIMER_H_
#define SOFTTIMER_H_

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
    uint32_t period;
    uint32_t counter;
    uint8_t mode;
    uint8_t state;
    uint8_t expired;
} softtimer_t;

/* ENUMS */

enum SOFTTIMER_MODE
{
    STM_ONESHOT         = 0,
    STM_PERIODIC        = 1
};

enum SOFTTIMER_STATE
{
    STS_STOPPED         = 0,
    STS_RUNNING         = 1,
    STS_EXPIRED         = 2
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t softtimerInit ( softtimer_t* driver, uint32_t period, uint8_t mode );
void softtimerStart ( softtimer_t* driver );
void softtimerStop ( softtimer_t* driver );
void softtimerTick ( softtimer_t* driver );
uint8_t softtimerExpired ( softtimer_t* driver );
uint8_t softtimerGetState ( const softtimer_t* const driver );
uint32_t softtimerGetElapsed ( const softtimer_t* const driver );
uint32_t softtimerGetRemaining ( const softtimer_t* const driver );
uint8_t softtimerChangePeriod ( softtimer_t* driver, uint32_t period );

#ifdef __cplusplus
}
#endif

#endif /* SOFTTIMER_H_ */
