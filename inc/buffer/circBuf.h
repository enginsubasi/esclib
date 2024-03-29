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
    uint8_t status;
} circBufu32_t;

/* ENUMS */

enum BUFFERSTATUS
{
    BS_EMPTY            = 0,
    BS_NOTEMPTY         = 1,
    BS_FULL             = 2,
};

enum BUFFERBEHAVIOUR
{
    BB_OVERWRITE        = 0,
    BB_STOP             = 1,
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t circBufInitu32 ( circBufu32_t* driver, uint32_t* buffer, uint32_t capacity, uint8_t behaviour );
uint32_t circBufGetLengthu32 ( circBufu32_t* driver );
uint8_t circBufGetStatusu32 ( circBufu32_t* driver );
uint8_t circBufAddu32 ( circBufu32_t* driver, uint32_t data );
uint8_t circBufReadu32 ( circBufu32_t* driver, uint32_t* data );

#ifdef __cplusplus
}
#endif

#endif /* CIRCBUF_H_ */
