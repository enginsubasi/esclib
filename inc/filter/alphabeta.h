#ifndef ALPHABETA_H_
#define ALPHABETA_H_

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
    float alpha;
    float betaOverDt;
    float dt;
    float position;
    float velocity;
} alphabeta_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t alphabetaInit ( alphabeta_t* driver, float alpha, float beta, float dt, float positionInit );
void alphabetaIteration ( alphabeta_t* driver, float measurement );
float alphabetaGetPosition ( const alphabeta_t* const driver );
float alphabetaGetVelocity ( const alphabeta_t* const driver );
float alphabetaGetPrediction ( const alphabeta_t* const driver, float ahead );

#ifdef __cplusplus
}
#endif

#endif /* ALPHABETA_H_ */
