#ifndef INC_DCMOTOR_H_
#define INC_DCMOTOR_H_

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
    void ( *bridgeHigh )( uint8_t );
    void ( *bridgeLow )( uint8_t );
    void ( *pwm )( float );
} dcmotor_t;

/* ENUMS */

enum BRIDGE_TYPE
{
    BRIDGE_NO       = 0,
    BRIDGE_FORWARD  = 1,
    BRIDGE_BACKWARD = 2,
    BRIDGE_LOCK     = 3,
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */
uint8_t dcMotorInit ( dcmotor_t *driver,
                    void ( *bridgeHighFnc )( uint8_t ),
                    void ( *bridgeLowFnc )( uint8_t ),
                    void ( *pwmFnc )( float ));
void dcMotorBridgeState ( dcmotor_t *driver, uint8_t bridgeState );

#ifdef __cplusplus
}
#endif

#endif /* INC_DCMOTOR_H_ */
