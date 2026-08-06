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

typedef struct
{
    int32_t alpha;
    int32_t beta;
    int64_t position;
    int64_t velocity;
} alphabetai32_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t alphabetaInit ( alphabeta_t* driver, float alpha, float beta, float dt, float positionInit );
void alphabetaIteration ( alphabeta_t* driver, float measurement );
float alphabetaGetPosition ( const alphabeta_t* const driver );
float alphabetaGetVelocity ( const alphabeta_t* const driver );
float alphabetaGetPrediction ( const alphabeta_t* const driver, float ahead );

uint8_t alphabetaIniti32 ( alphabetai32_t* driver, int32_t alpha, int32_t beta, int32_t positionInit );
void alphabetaIterationi32 ( alphabetai32_t* driver, int32_t measurement );
int32_t alphabetaGetPositioni32 ( const alphabetai32_t* const driver );
int32_t alphabetaGetVelocityi32 ( const alphabetai32_t* const driver );
int32_t alphabetaGetPredictioni32 ( const alphabetai32_t* const driver, int32_t ahead );

#ifdef __cplusplus
}
#endif

#endif /* ALPHABETA_H_ */
