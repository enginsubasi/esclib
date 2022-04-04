#ifndef CIRCBUF_H_
#define CIRCBUF_H_

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
    uint32_t capacity;
    uint32_t rp;
    uint32_t wp;
    uint8_t behaviour;
} circBufu32_t;

/* ENUMS */

enum BUFBEH
{
    BUFBEH_OVERWRITE        = 0,
    BUFBEH_STOP             = 1,
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

void circBufInitu32 ( circBufu32_t* driver, uint32_t* buffer, uint32_t size );
uint32_t circBufGetsizeu32 ( circBufu32_t* driver );

#ifdef __cplusplus
}
#endif

#endif /* CIRCBUF_H_ */   
