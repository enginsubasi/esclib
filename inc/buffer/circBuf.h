#ifndef GENERIC_H_
#define GENERIC_H_

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
    uint32_t* buffer;
    uint32_t size;
    
} circBufu32_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

#ifdef __cplusplus
}
#endif

#endif /* GENERIC_H_ */   
