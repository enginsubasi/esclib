#ifndef GENERIC_H_
#define GENERIC_H_

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
    uint32_t rxTimeoutCounter;
    uint32_t rxTimeout;
    
    uint32_t rxIndex;
    uint32_t txIndex;
    uint32_t rxSize;
    uint32_t txSize;
    
    uint8_t rxReadyToEvaluate;

    uint8_t *rxBuffer;
    
    uint8_t *txBuffer;
    
    void (*packetProcess) ( uint8_t* buffer, uint32_t index );
} comat_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

comatInit ( comat_t* driver, uint8_t* rxBuffer, uint8_t* txBuffer, 
                                uint32_t rxSize, uint32_t txSize, 
                                uint32_t rxTimeout, 
                                void (*packetProcess) ( uint8_t* buffer, uint32_t index ) );

#ifdef __cplusplus
}
#endif

#endif /* GENERIC_H_ */   
