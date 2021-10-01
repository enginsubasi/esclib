#ifndef CRC32_H_
#define CRC32_H_

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

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint32_t crc32 ( uint8_t* array, uint32_t size );

#ifdef __cplusplus
}
#endif

#endif /* CRC32_H_ */
