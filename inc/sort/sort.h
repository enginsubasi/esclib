#ifndef SORT_H_
#define SORT_H_

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

void sortSelection ( float* array, uint32_t length );
void sortSelectionu32 ( uint32_t* array, uint32_t length );
void sortSelectioni32 ( int32_t* array, uint32_t length );
void sortBubble ( float* array, uint32_t length );
void sortBubbleu32 ( uint32_t* array, uint32_t length );
void sortBubblei32 ( int32_t* array, uint32_t length );
void sortInsertion ( float* array, uint32_t length );
void sortInsertionu32 ( uint32_t* array, uint32_t length );
void sortInsertioni32 ( int32_t* array, uint32_t length );
void sortHeap ( float* array, uint32_t length );
void sortHeapu32 ( uint32_t* array, uint32_t length );
void sortHeapi32 ( int32_t* array, uint32_t length );
void sortReverse ( float* array, uint32_t length );
void sortReverseu32 ( uint32_t* array, uint32_t length );
void sortReversei32 ( int32_t* array, uint32_t length );
uint8_t sortIsSorted ( const float* const array, uint32_t length );
uint8_t sortIsSortedu32 ( const uint32_t* const array, uint32_t length );
uint8_t sortIsSortedi32 ( const int32_t* const array, uint32_t length );

#ifdef __cplusplus
}
#endif

#endif /* SORT_H_ */
