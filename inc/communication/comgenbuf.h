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

/* Longest packet the two byte length header can describe. */
#define COMGENBUF_MAX_PACKET    65535u

/* TYPEDEFS */

/* STRUCTURES */

typedef struct
{
    uint8_t* buffer;
    uint32_t size;
    uint32_t head;
    uint32_t tail;
    uint32_t used;
    uint32_t count;
    uint32_t dropCount;
} comgenbuf_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t comgenbufInit ( comgenbuf_t* driver, uint8_t* buffer, uint32_t size );
uint8_t comgenbufPush ( comgenbuf_t* driver, const uint8_t* const data, uint32_t length );
uint32_t comgenbufPeekLength ( const comgenbuf_t* const driver );
uint32_t comgenbufPop ( comgenbuf_t* driver, uint8_t* data, uint32_t capacity );
uint32_t comgenbufGetCount ( const comgenbuf_t* const driver );
uint32_t comgenbufGetFree ( const comgenbuf_t* const driver );
uint32_t comgenbufGetDropCount ( const comgenbuf_t* const driver );
void comgenbufFlush ( comgenbuf_t* driver );

#ifdef __cplusplus
}
#endif

#endif /* COMGENBUF_H_ */
