#ifndef CRC16_H_
#define CRC16_H_

#include <stdlib.h>

/* FUNCTION DEFINITIONS */

/* DEFINITIONS */

/* STRUCTURES */

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint16_t Calculate_CRC16 ( uint8_t* array, uint32_t size );
uint16_t Calculate_LUT_CRC16 ( uint8_t* array, uint32_t size );

#endif /* CRC16_H_ */
