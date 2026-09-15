#ifndef SCHED_H_
#define SCHED_H_

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
    void ( *task )( void );
    uint32_t period;
    uint32_t counter;
    uint8_t due;
    uint8_t enabled;
} schedTask_t;

typedef struct
{
    schedTask_t* tasks;
    uint32_t length;
    uint32_t count;
    uint32_t overrunCount;
} sched_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t schedInit ( sched_t* driver, schedTask_t* tasks, uint32_t length );
uint8_t schedAddTask ( sched_t* driver, void ( *task )( void ), uint32_t period, uint32_t* const index );
void schedTick ( sched_t* driver );
uint32_t schedRun ( sched_t* driver );
uint8_t schedEnable ( sched_t* driver, uint32_t index, uint8_t enabled );
uint32_t schedGetOverrunCount ( const sched_t* const driver );

#ifdef __cplusplus
}
#endif

#endif /* SCHED_H_ */
