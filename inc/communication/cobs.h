#ifndef COBS_H_
#define COBS_H_

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

/* The byte that never appears in an encoded buffer, and so is free to
   delimit one. */
#define COBS_DELIMITER      0x00u

/* The longest run of payload bytes one code byte can describe. */
#define COBS_RUN_MAX        254u

/* TYPEDEFS */

/* STRUCTURES */

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t cobsEncode ( const uint8_t* const source, uint32_t length,
                     uint8_t* destination, uint32_t capacity,
                     uint32_t* encodedLength );
uint8_t cobsDecode ( const uint8_t* const source, uint32_t length,
                     uint8_t* destination, uint32_t capacity,
                     uint32_t* decodedLength );
uint32_t cobsEncodedSize ( uint32_t length );

#ifdef __cplusplus
}
#endif

#endif /* COBS_H_ */
