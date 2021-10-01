#ifndef CRC16_H_
#define CRC16_H_

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

uint16_t crc16 ( uint8_t* array, uint32_t size );
uint16_t crc16Alt ( uint8_t* array, uint32_t size );

#ifdef __cplusplus
}
#endif

#endif /* CRC16_H_ */
