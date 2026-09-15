#ifndef FSM_H_
#define FSM_H_

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
    uint8_t from;
    uint8_t event;
    uint8_t to;
    void ( *action )( void );
} fsmTransition_t;

typedef struct
{
    const fsmTransition_t* table;
    uint32_t length;
    uint32_t rejectCount;
    uint8_t state;
    uint8_t initialState;
} fsm_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t fsmInit ( fsm_t* driver, const fsmTransition_t* const table, uint32_t length, uint8_t initialState );
uint8_t fsmDispatch ( fsm_t* driver, uint8_t event );
uint8_t fsmGetState ( const fsm_t* const driver );
uint32_t fsmGetRejectCount ( const fsm_t* const driver );
void fsmReset ( fsm_t* driver );

#ifdef __cplusplus
}
#endif

#endif /* FSM_H_ */
