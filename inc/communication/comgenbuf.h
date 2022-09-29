#ifndef COMGENBUF_H_
#define COMGENBUF_H_

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
    uint32_t index;     // Current index of the buffer
    uint32_t size;      // Size of the buffer
    uint8_t *buffer;    // Buffer
} comgenbuf_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

#ifdef __cplusplus
}
#endif

#endif /* COMGENBUF_H_ */   
