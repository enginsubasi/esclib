# ramp — setpoint profile with an acceleration limit

Date: 2026-08-05
Status: approved

## The problem

`slew` bounds one derivative. It moves its output toward a new sample by at most `maxStep` per call, which limits velocity and nothing else, and it has no notion of a target it is trying to *arrive at* — it only ever chases whatever it was last handed.

`pidControl` takes an error that is already computed, so setpoint generation lives entirely outside the library. Anything that needs a setpoint to accelerate, cruise and come to rest writes that profile by hand.

`ramp` fills the gap between them: it walks a setpoint toward a target under both a velocity and an acceleration limit, and it stops there.

## Identity: point to point, not a second-order filter

Two different things get called an acceleration limit, and the choice matters more than the arithmetic.

One is a **second-order tracking filter** — `slew` one derivative up. It follows its input under an acceleration bound and has no concept of arrival.

The other is a **point to point profile**. Given a target, it accelerates, cruises, and arrives with zero velocity. Knowing when to start braking is the whole problem, and it is what `slew` can never do.

This module is the second. It subsumes the first anyway: the target is a parameter of every iteration rather than something set once, so a moving target is handled by the same code path — the question the algorithm answers each call is "could I still stop at the target from here", and that question is well posed whether or not the target moved.

## Trapezoidal, not triangular

An acceleration limit alone produces a triangular profile on a long move: accelerate to the midpoint, then brake. Peak velocity grows without bound as the distance grows. That is valid, and it is almost never what a real axis or a real heater wants — those have a speed limit independent of how fast they can change speed.

So `ramp` carries both `maxVelocity` and `maxAcceleration`, and the profile is trapezoidal: accelerate to `maxVelocity`, cruise, brake.

The velocity cap cannot be bolted on from outside. Chaining `slew` after `ramp` breaks the braking arithmetic, because `ramp` computes its brake point from its *own* velocity while `slew` would be altering the actual velocity behind its back, and the target gets overshot. If the cap is wanted it has to be inside.

## Time base: `ts` at Init

The library has two precedents and they point opposite ways. `slew` takes `maxStep` per call, so the caller divides by the period and the module never learns about time. `pid` takes `ts` explicitly, and `pidInit` rejects `ts == 0` because `pidControl` divides by it.

`ramp` follows `pid`. Both of its limits are rates — `maxVelocity` in units per second, `maxAcceleration` in units per second squared — and with `ts` inside, the caller writes the numbers off the data sheet directly while the module does `velocity += a * ts` and `position += velocity * ts`. Without it the caller must pre-divide by the period and by the period squared, which is a needless conversion at every call site. `ramp` also lands in the same group as `pid`, so the consistency is local.

## Layout

```
inc/control/ramp.h
src/control/ramp.c
test/Ramp_Test/Ramp_Test.c
```

The module joins `pid` and `hysteresis` in the `control` group. Generating a setpoint is control rather than filtering, which is why it does not go next to `slew` in `filter`.

## Interface

```c
typedef struct
{
    float maxVelocity;
    float maxAcceleration;
    float ts;

    float position;
    float velocity;

    uint8_t arrived;
} ramp_t;
```

```c
uint8_t rampInit        ( ramp_t* driver, float maxVelocity, float maxAcceleration,
                          float ts, float positionInit );
void    rampIteration   ( ramp_t* driver, float target );
float   rampGetOutput   ( const ramp_t* const driver );
float   rampGetVelocity ( const ramp_t* const driver );
uint8_t rampIsArrived   ( const ramp_t* const driver );
```

The target is a parameter of `rampIteration`, not something installed at `Init`. That is exactly `slewIteration ( driver, newData )`'s shape, it removes the need for a separate `rampSetTarget`, and it is what makes a moving target fall out for free.

`rampIsArrived` follows the `sortIsSorted` naming precedent. Arrival is a level rather than an event — it holds until a new target moves the ramp again — so this is a plain predicate on a `const` driver, not a consuming read like `bininpGetRisingValue`.

Float only. `pid` is float only for the same reason, and the velocity envelope below is a square root, so an integer variant would not be the cheap transliteration `interp`'s was.

`rampChangeLimits` is deliberately absent. Overriding feed rate mid-move is a real requirement, but nothing asks for it yet and it can be added without disturbing anything here.

### rampInit

Returns `TRUE` on success and `FALSE` on a rejected argument, writing nothing to the driver on failure. It rejects a NULL `driver`, and a `maxVelocity`, `maxAcceleration` or `ts` that is not strictly positive. `positionInit` is unconstrained; `velocity` starts at zero and `arrived` at `TRUE`.

## The algorithm

Every `rampIteration` call is five steps.

```
remaining = target - position

vEnvelope = sqrtf ( 2 * maxAcceleration * |remaining| )
vDesired  = sign ( remaining ) * min ( maxVelocity, vEnvelope )

dv    = vDesired - velocity
maxDv = maxAcceleration * ts
velocity moves toward vDesired by at most maxDv

step = velocity * ts
if |step| >= |remaining|  ->  position = target; velocity = 0; arrived = TRUE
else                      ->  position += step;                arrived = FALSE
```

The whole module is the `vEnvelope` line. It answers "what is the fastest I could be going right now and still stop exactly at the target", which removes the need to compute a brake point separately — deceleration begins by itself the moment the velocity meets the envelope.

`sqrtf` comes from `<math.h>`, which `complex`, `biquad`, `basicmath` and `statistic` already include, and it is the single-precision form already used in this tree.

Three properties fall out of this form and are worth stating because they are what the implementation must not accidentally break.

**There is no division anywhere.** A square root and several multiplications, and no divide. So the zero rejections in `rampInit` are not there to keep a `nan` out of the output the way `pidInit`'s `ts` check is — they are there to stop the module from silently doing nothing, which is the reason `slewInit` rejects a `maxStep` of zero.

**Velocity needs no separate clamp.** `vDesired` is already bounded by `maxVelocity`, and the velocity step moves toward `vDesired` without passing it, so `|velocity| <= maxVelocity` is preserved inductively from the zero the driver starts at.

**It neither stalls nor fails to terminate.** While `remaining` is non-zero, `vDesired` is non-zero and the velocity is driven toward it, so the velocity cannot sit at zero. It rises by `maxAcceleration * ts` per step until it meets `vDesired`, which is `min ( maxVelocity, sqrtf ( 2 * maxAcceleration * remaining ) )`. Once `remaining` has fallen to `2 * maxAcceleration * ts * ts` or below, the envelope alone puts `|step|` at or above `remaining` and the final condition fires. Above that threshold the envelope exceeds `2 * maxAcceleration * ts`, so the velocity settles at `min ( maxVelocity, that )` — a positive constant — and each step shortens `remaining` by a positive constant, which reaches the threshold in finitely many steps.

The last line is discrete time's only correction. If the step would reach or pass the target, the position is placed exactly on the target and the velocity is zeroed. Without it the last step overshoots and the ramp oscillates around the target forever. It also handles the degenerate case at no extra cost: a target equal to the position gives `remaining` of zero and a `step` of zero, and `0 >= 0` arrives immediately.

A target moved backwards while the ramp is running will be overshot before the ramp turns around. That is physics rather than a defect — the module cannot teleport, and decelerating at `maxAcceleration` is the fastest it is permitted to stop.

## Testing

`test/Ramp_Test/Ramp_Test.c`, assert style, no `output.txt`, non-zero exit on failure.

| Case | What it proves |
|---|---|
| `Init` rejection | NULL driver, and `maxVelocity`, `maxAcceleration` and `ts` at zero and negative. The sentinel pattern confirms the driver is untouched on `FALSE` |
| Acceleration limit | From rest the first step's velocity is exactly `maxAcceleration * ts`, and consecutive steps grow by exactly that much |
| Velocity cap | On a long move the velocity reaches `maxVelocity` and never exceeds it |
| Arrival | The run converges to a position exactly equal to the target with a velocity of exactly zero, and `rampIsArrived` reports `TRUE` |
| **No overshoot** | Across the whole run the position never passes the target |
| Triangular profile | A move too short to reach `maxVelocity` never reaches it, and still arrives without overshoot |
| Negative direction | The mirror of the positive move, which catches a sign error |
| Moving target | The target changes mid-move and the ramp still arrives at the new one |
| Already at target | Arrival is immediate and the velocity stays at zero |

The pinned regression is **overshoot**. Without the final step clamp the ramp passes the target, comes back, and oscillates around it. Every other case in the file passes with that bug present, because they all measure either the convergence or the middle of the trajectory. The assertion is therefore checked on every step of the run rather than only at the end.

## Verification

- the tree-wide `-Wall -Wextra` sweep stays silent
- every header still coexists in one translation unit
- all five new exported symbols are referenced by the test
- exported symbol count 198 to 203
- test program count 22 to 23
- the assert-style test list in CLAUDE.md grows from fifteen to sixteen

## CLAUDE.md

- a paragraph in the module tour: `slew` bounds one derivative and has no target, `ramp` bounds two and comes to rest on one; the square-root envelope is what replaces an explicit brake point; the target is a per-call parameter, which is what makes a moving target free
- `ramp` added to the list of stateful modules that use their own name as a prefix
- the symbol, test and assert-style counts above
- a row in the pinned-bug table for the overshoot
