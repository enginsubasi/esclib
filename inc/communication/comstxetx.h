#ifndef COMSTXETX_H_
#define COMSTXETX_H_

#include <stdint.h>

/* FUNCTION DEFINITIONS */

/* DEFINITIONS */

#define RX_BUFFER_LENGTH    256
#define TX_BUFFER_LENGTH    256

/* TYPEDEFS */

/* STRUCTURES */

typedef struct
{
    uint8_t stx;
    uint8_t etx;
    
    uint32_t rxTimeoutCounter;
    uint32_t rxTimeout;
    
    uint32_t rxIndex;
    uint32_t rxMaxLength;
    
    uint8_t rxReadyToEvaluate;

    uint8_t rxBuffer[ RX_BUFFER_LENGTH ];
    
    uint8_t txBuffer[ TX_BUFFER_LENGTH ];
    
    void (*packetProcess) ( uint8_t* buffer, uint32_t index );
} com_stxetx_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

void comstxetxInit ( com_stxetx_t* driver );
void comstxetxReceive ( com_stxetx_t* driver, uint8_t data );
void comstxetxEvaluate ( com_stxetx_t* driver );
void comstxetxTimeoutCounter ( com_stxetx_t* driver );

#endif /* COMSTXETX_H_ */
