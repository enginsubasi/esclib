# Soft Timer Module for esclib

Date: 2026-08-04
Status: approved

## Problem

The library has no concept of time, yet two modules improvise one.

| Module | How it handles time today |
|---|---|
| `comat` / `comstxetx` | `xxxTimeoutCounter ( driver )` called from a periodic tick, counting into a private field |
| `hc595` / `hc597` | `dlyMs` / `dlyNop` callbacks injected at `Init`, or one step per ISR call in interrupt mode |

Each one re-derives the same shape: a counter advanced from a fixed-rate interrupt, compared against a limit, read from the main loop. Nothing exports that shape, so a caller who needs a plain "has 250 ms passed yet" has to write it again.

`softtimer` is that shape, extracted and made general. It is additive: no existing module changes, and none of them may consume it — module independence forbids `comat` from including `softtimer.h`.

## Decisions

Four questions were settled before design.

1. **The prefix is `softtimer`, not `timer`.** `timerInit` is a prime candidate for a collision with a target project's HAL or RTOS, which is the exact failure the library's prefix rule exists to prevent. Length is not an objection — `alphabeta` and `comstxetx` are already nine characters.

2. **The module counts ticks; it does not compare timestamps.** `softtimerTick ( driver )` is called from a fixed-rate ISR and advances the counter by one. The period is expressed in ticks, so the interrupt period is the unit — the same rule CLAUDE.md already states for `hc595Interrupt`: "the interrupt period *is* the timing".

   The alternative was a free-running counter with `now - start` comparison. It is cheaper — no work in the ISR, and N timers cost nothing — but it assumes hardware that esclib does not assume. It could have been injected as `uint32_t (*getTick) ( void )`, but that binds a single-purpose module to a callback and makes the timer unreadable unless that callback is live.

   The accepted cost: N timers means N calls per ISR. That is acceptable for a handful of timers, and hundreds of timers is not this library's problem.

3. **`softtimerExpired` consumes the flag it reports.** The library already has a precedent for a consuming read — `bininpGetRisingValue` clears the flag it returns, and CLAUDE.md marks it as genuinely `in,out`. `softtimerExpired` follows it exactly: it takes a non-`const` driver and is documented `@param[in,out]`.

4. **The reload happens inside the tick rather than at the read.** This is what preserves the phase when the main loop misses an expiry. The reload itself subtracts the period rather than clearing the counter, a form that stays correct if the counter ever overshoots today's invariant that it lands exactly on the period. The number of missed expiries is not tracked — deliberately, as no caller in view needs it.

## Placement

```
inc/timer/softtimer.h   ←→   src/timer/softtimer.c
```

A new `timer/` group rather than a home under `control/`: this is a time base, not a controller.

The header is a copy of `template/inc/generic.h` with content filled into the fixed sections, empty sections preserved. The source opens with the banner from `template/src/generic.c`, `@version 1.0.0` and a `04/08/2026` line in `@par History`.

## API

```c
uint8_t  softtimerInit ( softtimer_t* driver, uint32_t period, uint8_t mode );
void     softtimerStart ( softtimer_t* driver );
void     softtimerStop ( softtimer_t* driver );
void     softtimerTick ( softtimer_t* driver );
uint8_t  softtimerExpired ( softtimer_t* driver );
uint8_t  softtimerGetState ( const softtimer_t* const driver );
uint32_t softtimerGetElapsed ( const softtimer_t* const driver );
uint32_t softtimerGetRemaining ( const softtimer_t* const driver );
uint8_t  softtimerChangePeriod ( softtimer_t* driver, uint32_t period );
```

Nine exported symbols.

`softtimerStart` returns `void`. It takes no new argument and has no state it must refuse from — `hc595Start` returns a status because two transfer modes contend for one set of pins, and `softtimer` has no such contention. CLAUDE.md's rule that validation happens at `Init` and nowhere else makes this `void`.

`softtimerStart` resets the counter and the flag, so it is also the restart. There is no separate `Restart`.

`softtimerChangePeriod` returns a status because it takes a *new* argument that can break a later invariant — the same reason `pidChangeCoefficients` returns one for `ts`.

`softtimerTick` and `softtimerExpired` perform no argument checks. They are the ISR and main-loop hot paths; the contract was settled at `Init`.

## State

```c
typedef struct
{
    uint32_t period;
    uint32_t counter;
    uint8_t  mode;
    uint8_t  state;
    uint8_t  expired;
} softtimer_t;
```

```c
enum SOFTTIMER_MODE
{
    STM_ONESHOT         = 0,
    STM_PERIODIC        = 1
};

enum SOFTTIMER_STATE
{
    STS_STOPPED         = 0,
    STS_RUNNING         = 1,
    STS_EXPIRED         = 2
};
```

The enums are not typedef'd, matching `BUFFERSTATUS`, `BUFFERBEHAVIOUR` and `HC595_STATE`, and the members are numbered explicitly. The two enums take separate member prefixes — `STM_` and `STS_` — the way `circBuf` separates `BS_` from `BB_`. `mode` and `state` are carried in the struct as `uint8_t`, and `softtimerGetState` returns `uint8_t`, as `hc595GetState` does.

## Behaviour

Order matters here, so the whole state machine is stated in one place.

- `Init` leaves the timer `STS_STOPPED`. It does not start on its own; `Start` is required. This is the point most likely to be misread, and the test pins it first.
- `Tick` does nothing unless the state is `STS_RUNNING`. Every timer can therefore be ticked unconditionally from the ISR.
- Expiry is `counter >= period` after the increment, so a timer initialised with period N expires on the Nth tick after `Start`.
- One-shot expiry: `expired = TRUE`, state becomes `STS_EXPIRED`, and **the counter stops advancing**. That is what keeps the counter from overflowing.
- Periodic expiry: `expired = TRUE`, `counter -= period`, state stays `STS_RUNNING`. `STS_EXPIRED` is reachable in one-shot mode only.
- `Start` clears the counter and the flag and sets `STS_RUNNING`.
- `Stop` freezes: state becomes `STS_STOPPED`, counter and flag are left alone, so `GetElapsed` still means something after a stop. The split is **Stop freezes, Start resets**.
- `softtimerGetState` reports the condition without consuming it, the way `hc595GetState` reports `IDLE`/`BUSY`/`BLOCKING`/`DONE`. A one-shot stays `STS_EXPIRED` after `softtimerExpired` has taken the flag: the state is the condition, the flag is the event.

## Validation

`softtimerInit` checks, and writes nothing to the driver when any check fails:

- `driver != NULL`, using `NULL` from `<stddef.h>`, never a bare `0`.
- `period != 0`. A zero period expires on every tick, and the subtracting reload never converges. Same reasoning as `pidInit` rejecting `ts == 0`.
- `mode` is one of `STM_ONESHOT` or `STM_PERIODIC`.

On success: `period` and `mode` are stored, `counter = 0`, `expired = FALSE`, `state = STS_STOPPED`.

`softtimerChangePeriod` checks `driver != NULL` and `period != 0`. On success it writes the period and **resets the counter to zero**, leaving state and flag untouched. Without the reset, a new period smaller than the current counter would expire on consecutive ticks until the counter drained.

`softtimerGetRemaining` returns `0` once the counter has reached the period, computed with an explicit comparison rather than an unsigned subtraction that would wrap.

`softtimerGetElapsed` and `softtimerGetRemaining` both stay in the API. `period` is not exposed, so neither is derivable from the other through the public interface.

## Testing

Assert style — `test/SoftTimer_Test/SoftTimer_Test.c`, no `output.txt`, non-zero return on failure.

1. `Init` rejects a NULL driver, `period == 0`, and an out-of-range mode. A sentinel pattern written into the struct beforehand is unchanged after every rejected call.
2. `Init` leaves `STS_STOPPED`; ticking a stopped timer `period + 10` times leaves `Expired` `FALSE`.
3. A one-shot of period N expires on the Nth tick, not the (N-1)th; `Expired` reads `TRUE` once then `FALSE`; the state stays `STS_EXPIRED`; further ticks do not expire it again.
4. **The pinned regressions.** `periodicCase` pins that the mode split exists: a periodic timer reloads and stays running rather than reaching `STS_EXPIRED` like a one-shot. `periodicPhaseCase` pins that the reload happens at all: period 10, tick 35 without reading the flag, read `Expired` once, then confirm the next expiry lands on tick 40. With no reload at all, the counter would stay at 35 and never expire again. The case does not distinguish subtracting the period from clearing the counter — both are equivalent here, because the counter is always below the period on entry and lands exactly on it.
5. `Start` clears the counter and the flag; after `Stop`, further ticks leave `GetElapsed` unchanged.
6. `GetRemaining` returns `0` after expiry and does not wrap.
7. `ChangePeriod` rejects zero and leaves the driver intact; on success it resets the counter.

## Repository updates

- New `inc/timer/` and `src/timer/` directories.
- CLAUDE.md: add the module to the overview, test count 20 to 21, exported symbol count 181 to 190.
- Verification unchanged: clean under `-Wall -Wextra`, and the new header must coexist with every other header in one translation unit.
