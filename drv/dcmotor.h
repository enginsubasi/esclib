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

#define DEF_DLY_COUNT   1 // In millisecond.

/* TYPEDEFS */

/* STRUCTURES */


struct dcmotor_t
{

};

/* ENUMS */

enum DLY_TYPE
{
    DLY_NO              = 0,
    DLY_MS              = 1,
    DLY_NOP             = 2
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */
void dcmotorInit ( struct dcmotor_t *driver );

#ifdef __cplusplus
}
#endif

#endif /* INC_HC595_DRV_H_ */
