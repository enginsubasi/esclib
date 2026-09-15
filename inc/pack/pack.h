#ifndef PACK_H_
#define PACK_H_

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

void packPutU16be ( uint8_t* const buffer, uint32_t index, uint16_t value );
void packPutU16le ( uint8_t* const buffer, uint32_t index, uint16_t value );
void packPutU32be ( uint8_t* const buffer, uint32_t index, uint32_t value );
void packPutU32le ( uint8_t* const buffer, uint32_t index, uint32_t value );

uint16_t packGetU16be ( const uint8_t* const buffer, uint32_t index );
uint16_t packGetU16le ( const uint8_t* const buffer, uint32_t index );
uint32_t packGetU32be ( const uint8_t* const buffer, uint32_t index );
uint32_t packGetU32le ( const uint8_t* const buffer, uint32_t index );

int16_t packGetI16be ( const uint8_t* const buffer, uint32_t index );
int16_t packGetI16le ( const uint8_t* const buffer, uint32_t index );
int32_t packGetI32be ( const uint8_t* const buffer, uint32_t index );
int32_t packGetI32le ( const uint8_t* const buffer, uint32_t index );

uint32_t packGetU24be ( const uint8_t* const buffer, uint32_t index );
uint32_t packGetU24le ( const uint8_t* const buffer, uint32_t index );
int32_t packGetI24be ( const uint8_t* const buffer, uint32_t index );
int32_t packGetI24le ( const uint8_t* const buffer, uint32_t index );

#ifdef __cplusplus
}
#endif

#endif /* PACK_H_ */
