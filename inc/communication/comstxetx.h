#ifndef COMSTXETX_H_
#define COMSTXETX_H_

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
    uint8_t stx;
    uint8_t etx;
    
    uint32_t rxTimeoutCounter;
    uint32_t rxTimeout;
    
    uint32_t rxIndex;
    uint32_t rxSize;
    uint32_t txSize;
    
    uint8_t rxReadyToEvaluate;

    uint8_t *rxBuffer;
    
    uint8_t *txBuffer;
    
    void (*packetProcess) ( uint8_t* buffer, uint32_t index );
} com_stxetx_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

void comstxetxInit ( com_stxetx_t* driver, uint8_t* rxBuffer, uint8_t* txBuffer uint32_t rxSize, uint32_t txSize, uint8_t stx, uint8_t etx, uint32_t rxTimeout, void (*packetProcess) ( uint8_t* buffer, uint32_t index ) );
void comstxetxReceive ( com_stxetx_t* driver, uint8_t data );
void comstxetxEvaluate ( com_stxetx_t* driver );
void comstxetxTimeoutCounter ( com_stxetx_t* driver );

#ifdef __cplusplus
}
#endif

#endif /* COMSTXETX_H_ */
