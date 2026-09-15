#ifndef TEXT_H_
#define TEXT_H_

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

/* The longest text each decimal writer can produce, for sizing a buffer. */
#define TEXT_U32_MAX            10u
#define TEXT_I32_MAX            11u

/* textFromQ16 writes at most this many decimals, and at that many the text
   is at most TEXT_Q16_MAX bytes: a sign, five digits, a point, nine more. */
#define TEXT_Q16_DECIMALS_MAX   9u
#define TEXT_Q16_MAX            16u

/* TYPEDEFS */

/* STRUCTURES */

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t textHexEncode ( const uint8_t* const source, uint32_t length,
                        uint8_t* destination, uint32_t capacity,
                        uint32_t* written );
uint8_t textHexDecode ( const uint8_t* const source, uint32_t length,
                        uint8_t* destination, uint32_t capacity,
                        uint32_t* written );
uint32_t textBase64EncodedSize ( uint32_t length );
uint8_t textBase64Encode ( const uint8_t* const source, uint32_t length,
                           uint8_t* destination, uint32_t capacity,
                           uint32_t* written );
uint8_t textBase64Decode ( const uint8_t* const source, uint32_t length,
                           uint8_t* destination, uint32_t capacity,
                           uint32_t* written );
uint8_t textFromU32 ( uint32_t value, uint8_t* destination, uint32_t capacity,
                      uint32_t* written );
uint8_t textFromI32 ( int32_t value, uint8_t* destination, uint32_t capacity,
                      uint32_t* written );
uint8_t textFromQ16 ( int32_t value, uint32_t decimals, uint8_t* destination,
                      uint32_t capacity, uint32_t* written );
uint8_t textToU32 ( const uint8_t* const source, uint32_t length,
                    uint32_t* value );
uint8_t textToI32 ( const uint8_t* const source, uint32_t length,
                    int32_t* value );
uint8_t textToQ16 ( const uint8_t* const source, uint32_t length,
                    int32_t* value );

#ifdef __cplusplus
}
#endif

#endif /* TEXT_H_ */
