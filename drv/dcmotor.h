#ifndef INC_HC595_DRV_H_
#define INC_HC595_DRV_H_

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


struct dcmotor_t
{
    void ( *bridgeHigh )( uint8_t );
    void ( *bridgeLow )( uint8_t );
    void ( *pwm )( double );
};

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
void dcmotorInit ( struct dcmotor_t *driver,
                    void ( *bridgeHighFnc )( uint8_t ),
                    void ( *bridgeLowFnc )( uint8_t ),
                    void ( *pwmFnc )( double ));
void dcMotorBridgeState ( struct dcmotor_t *driver, uint8_t bridgeState );

#ifdef __cplusplus
}
#endif

#endif /* INC_HC595_DRV_H_ */
