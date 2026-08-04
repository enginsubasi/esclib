# Soft Timer Module Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a `softtimer` module that turns a fixed-rate ISR tick into a one-shot or periodic timeout any caller can poll, filling the library's missing time abstraction.

**Architecture:** A single self-contained driver-struct module, `inc/timer/softtimer.h` ↔ `src/timer/softtimer.c`. The caller owns the `softtimer_t`, calls `softtimerTick` from a fixed-rate interrupt and `softtimerExpired` from the main loop. Nothing else in the tree changes: module independence forbids `comat`, `dcMotor` or anyone else from consuming it.

**Tech Stack:** Freestanding C89-style C, `<stdint.h>` types, `<stddef.h>` for `NULL`. No heap, no OS, no build system. Verified with `arm-none-eabi-gcc`.

Spec: `docs/superpowers/specs/2026-08-04-softtimer-design.md`

## Global Constraints

Every task's requirements implicitly include this section.

- **No host compiler on this machine.** `gcc`, `clang`, `cc` and `tcc` are all absent; only `arm-none-eabi-gcc` exists, and it cross-compiles for ARM so its output cannot be executed here. Every verification step below is compile-time or link-time. **Never claim the tests passed** — say they compile and link, and that execution is pending a machine with a host compiler.
- Zero warnings under `-Wall -Wextra`. A new warning is a regression, not noise.
- `src/timer/softtimer.c` includes only `<stddef.h>` and `"softtimer.h"`. No other module's header, ever.
- Every exported symbol starts with `softtimer`. Nine of them, no more.
- Style, enforced exactly (`codingReference.md`):
  - Spaces inside every paren: `if ( ( a > b ) || ( c == d ) )`, `foo ( &driver, 5 )`.
  - Allman braces, braces on every block including single statements.
  - Pre-increment: `++driver->counter`.
  - One `retVal` local, initialized at its declaration, single exit written `return ( retVal );`.
  - Status returns use `TRUE`/`FALSE`, never `0`/`1` literals.
  - Empty `else` branches written out with `/* Intentionally blank */`, never omitted.
  - `NULL` from `<stddef.h>`, never a bare `0`.
- Documentation lives in the `.c` only. The header is a pure declaration file: a copy of `template/inc/generic.h` with content filled into the fixed sections, **all sections preserved including the empty ones**.
- Validation happens in `softtimerInit` and `softtimerChangePeriod` and nowhere else. `softtimerTick`, `softtimerExpired`, `softtimerStart`, `softtimerStop` and the getters dereference `driver` unchecked — they are the ISR and main-loop hot paths.
- Commit messages are terse and prefixed: `+` for additions, `*` for fixes. **Never** add a `Co-Authored-By:` trailer, a "Generated with Claude Code" footer, or any other AI attribution.

### Verification commands

Compile the module (run after every implementation step):

```bash
arm-none-eabi-gcc -c -Wall -Wextra -Iinc/timer src/timer/softtimer.c -o /dev/null
```

Link the test (the red/green gate; newlib prints `_close is not implemented` style warnings from its own syscall stubs — those are expected and are not from our code):

```bash
arm-none-eabi-gcc -Wall -Wextra -Iinc/timer --specs=nosys.specs \
  test/SoftTimer_Test/SoftTimer_Test.c src/timer/softtimer.c -o /tmp/softtimer_test.elf
```

Execute (**not possible on this machine** — for a host with a real `gcc`):

```bash
gcc -Wall -Wextra -Iinc/timer test/SoftTimer_Test/SoftTimer_Test.c src/timer/softtimer.c -o softtimer_test && ./softtimer_test
```

## File Structure

| File | Responsibility |
|---|---|
| `inc/timer/softtimer.h` | Create. Struct, two enums, nine prototypes. No documentation. |
| `src/timer/softtimer.c` | Create. All nine functions plus the Doxygen banner and per-function blocks. |
| `test/SoftTimer_Test/SoftTimer_Test.c` | Create. Assert-style, one `main`, no `output.txt`, returns non-zero on failure. Grows across Tasks 1-4. |
| `CLAUDE.md` | Modify. Module overview, test count, exported symbol count. |

---

### Task 1: Header, `softtimerInit`, `softtimerGetState`

The Init contract and the initial state. Nothing counts yet.

**Files:**
- Create: `inc/timer/softtimer.h`
- Create: `src/timer/softtimer.c`
- Test: `test/SoftTimer_Test/SoftTimer_Test.c`

**Interfaces:**
- Consumes: nothing.
- Produces: `softtimer_t`; `enum SOFTTIMER_MODE { STM_ONESHOT = 0, STM_PERIODIC = 1 }`; `enum SOFTTIMER_STATE { STS_STOPPED = 0, STS_RUNNING = 1, STS_EXPIRED = 2 }`; `uint8_t softtimerInit ( softtimer_t* driver, uint32_t period, uint8_t mode )`; `uint8_t softtimerGetState ( const softtimer_t* const driver )`.

- [ ] **Step 1: Create the header**

Create `inc/timer/softtimer.h`:

```c
#ifndef SOFTTIMER_H_
#define SOFTTIMER_H_

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
    uint32_t period;
    uint32_t counter;
    uint8_t mode;
    uint8_t state;
    uint8_t expired;
} softtimer_t;

/* ENUMS */

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

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t softtimerInit ( softtimer_t* driver, uint32_t period, uint8_t mode );
void softtimerStart ( softtimer_t* driver );
void softtimerStop ( softtimer_t* driver );
void softtimerTick ( softtimer_t* driver );
uint8_t softtimerExpired ( softtimer_t* driver );
uint8_t softtimerGetState ( const softtimer_t* const driver );
uint32_t softtimerGetElapsed ( const softtimer_t* const driver );
uint32_t softtimerGetRemaining ( const softtimer_t* const driver );
uint8_t softtimerChangePeriod ( softtimer_t* driver, uint32_t period );

#ifdef __cplusplus
}
#endif

#endif /* SOFTTIMER_H_ */
```

All nine prototypes go in now even though Tasks 2-4 implement most of them. The header is the module's contract and splitting it across tasks would leave it inconsistent with itself between commits.

- [ ] **Step 2: Write the failing test**

Create `test/SoftTimer_Test/SoftTimer_Test.c`:

```c
/*
 * Soft timer module test.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero when any check fails.
 *
 * periodicCase and periodicPhaseCase pin the periodic mode split: that a
 * periodic timer reloads and keeps running rather than stopping at its first
 * expiry the way a one shot does, and that the reload happens at all. They do
 * not distinguish subtracting the period from clearing the counter, and no
 * test of this API can: the counter is below the period on entry to every
 * tick, so it lands exactly on the period and the two reloads agree.
 */

#include <stdio.h>

#include "softtimer.h"

#define SENTINEL32  0xA5A5A5A5u
#define SENTINEL8   0xA5u

static uint32_t failures = 0;

static void check ( const char* what, uint8_t condition )
{
    if ( condition == TRUE )
    {
        printf ( "  PASS  %s\n", what );
    }
    else
    {
        printf ( "  FAIL  %s\n", what );
        ++failures;
    }
}

/*
 * A rejected Init must leave the driver exactly as it found it, so the caller
 * cannot half initialize a timer by ignoring the return value. The pattern is
 * written by hand rather than with memset to keep the test free of string.h.
 */
static void fillSentinel ( softtimer_t* driver )
{
    driver->period = SENTINEL32;
    driver->counter = SENTINEL32;
    driver->mode = SENTINEL8;
    driver->state = SENTINEL8;
    driver->expired = SENTINEL8;
}

static uint8_t isSentinel ( const softtimer_t* const driver )
{
    uint8_t retVal = FALSE;

    if ( ( driver->period == SENTINEL32 ) && ( driver->counter == SENTINEL32 ) &&
         ( driver->mode == SENTINEL8 ) && ( driver->state == SENTINEL8 ) &&
         ( driver->expired == SENTINEL8 ) )
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/* ------------------------------------------------------- the Init contract */

static void initCase ( void )
{
    softtimer_t driver;

    printf ( "softtimerInit\n" );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( softtimerInit ( NULL, 10u, ( uint8_t ) STM_ONESHOT ) == FALSE ) );

    fillSentinel ( &driver );
    check ( "a zero period is rejected",
            ( uint8_t ) ( softtimerInit ( &driver, 0u, ( uint8_t ) STM_ONESHOT ) == FALSE ) );
    check ( "the rejected zero period left the driver untouched", isSentinel ( &driver ) );

    fillSentinel ( &driver );
    check ( "an out of range mode is rejected",
            ( uint8_t ) ( softtimerInit ( &driver, 10u, 7u ) == FALSE ) );
    check ( "the rejected mode left the driver untouched", isSentinel ( &driver ) );

    check ( "one shot init succeeds",
            softtimerInit ( &driver, 10u, ( uint8_t ) STM_ONESHOT ) );
    check ( "init leaves the timer stopped",
            ( uint8_t ) ( softtimerGetState ( &driver ) == ( uint8_t ) STS_STOPPED ) );

    check ( "periodic init succeeds",
            softtimerInit ( &driver, 10u, ( uint8_t ) STM_PERIODIC ) );
    check ( "init leaves the periodic timer stopped too",
            ( uint8_t ) ( softtimerGetState ( &driver ) == ( uint8_t ) STS_STOPPED ) );
}

int main ( void )
{
    initCase ( );

    printf ( "\n" );

    if ( failures == 0 )
    {
        printf ( "all checks passed\n" );
    }
    else
    {
        printf ( "%lu check(s) failed\n", ( unsigned long ) failures );
    }

    return ( ( failures == 0 ) ? 0 : 1 );
}
```

`NULL` reaches this file through `<stdio.h>`, so the test needs no `<stddef.h>` of its own.

- [ ] **Step 3: Create the source file with the banner only, then link to verify the test fails**

Create `src/timer/softtimer.c` containing only the banner and includes:

```c
/**
  ******************************************************************************
  *
  * @file      softtimer.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      04/08/2026
  *
  * @brief     Tick counting soft timer, one shot or periodic.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 04/08/2026 Created. @n
  *
  * @note      The period is expressed in ticks, not in milliseconds. The rate
  *            at which softtimerTick is called is the unit, so a 250 ms
  *            timeout on a 1 kHz interrupt is a period of 250.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "softtimer.h"
```

Run:

```bash
arm-none-eabi-gcc -Wall -Wextra -Iinc/timer --specs=nosys.specs \
  test/SoftTimer_Test/SoftTimer_Test.c src/timer/softtimer.c -o /tmp/softtimer_test.elf
```

Expected: FAIL, `undefined reference to 'softtimerInit'` and `undefined reference to 'softtimerGetState'`.

- [ ] **Step 4: Implement `softtimerInit` and `softtimerGetState`**

Append to `src/timer/softtimer.c`:

```c
/**
 * @brief   Initializes a soft timer and leaves it stopped.
 * @param[out] driver  Timer state to initialize.
 * @param[in]  period  Number of softtimerTick calls that make up one timeout.
 * @param[in]  mode    STM_ONESHOT or STM_PERIODIC.
 * @return  TRUE on success, FALSE when driver is NULL, period is zero or mode
 *          is neither of the two accepted values. Nothing is written to the
 *          driver when FALSE is returned.
 * @note    The timer does not start here. softtimerStart is what sets it
 *          running.
 * @note    A zero period is rejected because it would expire on every tick,
 *          and in periodic mode the subtracting reload would never converge.
 */
uint8_t softtimerInit ( softtimer_t* driver, uint32_t period, uint8_t mode )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) &&
         ( period > 0u ) &&
         ( ( mode == ( uint8_t ) STM_ONESHOT ) || ( mode == ( uint8_t ) STM_PERIODIC ) ) )
    {
        driver->period = period;
        driver->counter = 0u;
        driver->mode = mode;
        driver->state = ( uint8_t ) STS_STOPPED;
        driver->expired = FALSE;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether the timer is stopped, running or finished.
 * @param[in] driver  Timer state.
 * @return  STS_STOPPED, STS_RUNNING or STS_EXPIRED.
 * @note    This reports the condition and does not consume it. STS_EXPIRED is
 *          reachable in one shot mode only; a periodic timer stays
 *          STS_RUNNING across its expiries.
 */
uint8_t softtimerGetState ( const softtimer_t* const driver )
{
    uint8_t retVal = ( uint8_t ) STS_STOPPED;

    retVal = driver->state;

    return ( retVal );
}
```

- [ ] **Step 5: Compile and link to verify it passes**

Run:

```bash
arm-none-eabi-gcc -c -Wall -Wextra -Iinc/timer src/timer/softtimer.c -o /dev/null
arm-none-eabi-gcc -Wall -Wextra -Iinc/timer --specs=nosys.specs \
  test/SoftTimer_Test/SoftTimer_Test.c src/timer/softtimer.c -o /tmp/softtimer_test.elf
```

Expected: no warnings from either command, and the link succeeds. Only the newlib `_close`/`_read`/`_write is not implemented` stub warnings appear, which come from the C library, not from this code.

The remaining seven prototypes are declared but not yet defined. That is fine — the test does not call them yet, so nothing is undefined at link time.

- [ ] **Step 6: Commit**

```bash
git add inc/timer/softtimer.h src/timer/softtimer.c test/SoftTimer_Test/SoftTimer_Test.c
git commit -m "+ Soft timer module skeleton and its init contract"
```

---

### Task 2: One-shot counting — `softtimerStart`, `softtimerStop`, `softtimerTick`, `softtimerExpired`

The working timer. Periodic reload is deliberately left to Task 3.

**Files:**
- Modify: `src/timer/softtimer.c`
- Test: `test/SoftTimer_Test/SoftTimer_Test.c`

**Interfaces:**
- Consumes: `softtimer_t`, `softtimerInit`, `softtimerGetState`, `STM_ONESHOT`, `STS_STOPPED`, `STS_RUNNING`, `STS_EXPIRED` from Task 1.
- Produces: `void softtimerStart ( softtimer_t* driver )`; `void softtimerStop ( softtimer_t* driver )`; `void softtimerTick ( softtimer_t* driver )`; `uint8_t softtimerExpired ( softtimer_t* driver )`.

- [ ] **Step 1: Write the failing test**

Insert these two functions into `test/SoftTimer_Test/SoftTimer_Test.c` above `main`:

```c
/* --------------------------------------------------------- one shot timing */

/*
 * A period of N must expire on the Nth tick after Start, not the N-1th, and
 * the flag must read TRUE exactly once. Once expired the counter stops, which
 * is what keeps a long lived one shot from wrapping its counter.
 */
static void oneShotCase ( void )
{
    softtimer_t driver;
    uint32_t i = 0;

    printf ( "one shot timing\n" );

    check ( "init", softtimerInit ( &driver, 10u, ( uint8_t ) STM_ONESHOT ) );

    softtimerStart ( &driver );
    check ( "start sets the timer running",
            ( uint8_t ) ( softtimerGetState ( &driver ) == ( uint8_t ) STS_RUNNING ) );

    for ( i = 0; i < 9u; ++i )
    {
        softtimerTick ( &driver );
    }

    check ( "not expired after 9 of 10 ticks",
            ( uint8_t ) ( softtimerExpired ( &driver ) == FALSE ) );

    softtimerTick ( &driver );

    check ( "state is expired after the 10th tick",
            ( uint8_t ) ( softtimerGetState ( &driver ) == ( uint8_t ) STS_EXPIRED ) );
    check ( "the flag reads TRUE once",
            ( uint8_t ) ( softtimerExpired ( &driver ) == TRUE ) );
    check ( "and FALSE on the second read",
            ( uint8_t ) ( softtimerExpired ( &driver ) == FALSE ) );
    check ( "the state survives the flag being consumed",
            ( uint8_t ) ( softtimerGetState ( &driver ) == ( uint8_t ) STS_EXPIRED ) );

    for ( i = 0; i < 100u; ++i )
    {
        softtimerTick ( &driver );
    }

    check ( "a finished one shot does not expire again",
            ( uint8_t ) ( softtimerExpired ( &driver ) == FALSE ) );
}

/* ------------------------------------------------------- start, stop, ticks */

/*
 * Stop freezes, Start resets. A stopped timer must ignore ticks outright so
 * the caller can tick every timer it owns from the ISR without asking which
 * ones are live.
 */
static void startStopCase ( void )
{
    softtimer_t driver;
    uint32_t i = 0;

    printf ( "start and stop\n" );

    check ( "init", softtimerInit ( &driver, 10u, ( uint8_t ) STM_ONESHOT ) );

    for ( i = 0; i < 20u; ++i )
    {
        softtimerTick ( &driver );
    }

    check ( "an unstarted timer ignores ticks",
            ( uint8_t ) ( softtimerExpired ( &driver ) == FALSE ) );
    check ( "and stays stopped",
            ( uint8_t ) ( softtimerGetState ( &driver ) == ( uint8_t ) STS_STOPPED ) );

    softtimerStart ( &driver );

    for ( i = 0; i < 5u; ++i )
    {
        softtimerTick ( &driver );
    }

    softtimerStop ( &driver );

    for ( i = 0; i < 20u; ++i )
    {
        softtimerTick ( &driver );
    }

    check ( "a stopped timer ignores ticks",
            ( uint8_t ) ( softtimerExpired ( &driver ) == FALSE ) );

    softtimerStart ( &driver );

    for ( i = 0; i < 9u; ++i )
    {
        softtimerTick ( &driver );
    }

    check ( "start reset the counter, so 9 ticks is not enough",
            ( uint8_t ) ( softtimerExpired ( &driver ) == FALSE ) );

    softtimerTick ( &driver );

    check ( "and the 10th tick after start expires it",
            ( uint8_t ) ( softtimerExpired ( &driver ) == TRUE ) );
}
```

Add the calls to `main`, immediately after the existing `initCase ( );` line:

```c
    printf ( "\n" );
    oneShotCase ( );
    printf ( "\n" );
    startStopCase ( );
```

- [ ] **Step 2: Link to verify it fails**

Run:

```bash
arm-none-eabi-gcc -Wall -Wextra -Iinc/timer --specs=nosys.specs \
  test/SoftTimer_Test/SoftTimer_Test.c src/timer/softtimer.c -o /tmp/softtimer_test.elf
```

Expected: FAIL, `undefined reference to 'softtimerStart'`, `'softtimerStop'`, `'softtimerTick'`, `'softtimerExpired'`.

- [ ] **Step 3: Implement the four functions**

Append to `src/timer/softtimer.c`:

```c
/**
 * @brief   Starts the timer from zero.
 * @param[in,out] driver  Timer state.
 * @note    This is also the restart. The counter and the expiry flag are both
 *          cleared, so there is no separate Restart function.
 */
void softtimerStart ( softtimer_t* driver )
{
    driver->counter = 0u;
    driver->expired = FALSE;
    driver->state = ( uint8_t ) STS_RUNNING;
}

/**
 * @brief   Stops the timer without disturbing what it has counted.
 * @param[in,out] driver  Timer state.
 * @note    Stop freezes and Start resets. The counter and the expiry flag are
 *          left alone here, so softtimerGetElapsed still means something after
 *          a stop.
 */
void softtimerStop ( softtimer_t* driver )
{
    driver->state = ( uint8_t ) STS_STOPPED;
}

/**
 * @brief   Advances the timer by one tick. Call this from a fixed rate ISR.
 * @param[in,out] driver  Timer state.
 * @note    A timer that is not running ignores the call, so every timer the
 *          caller owns can be ticked unconditionally.
 * @note    A one shot stops counting once it expires, which is what keeps its
 *          counter from wrapping.
 */
void softtimerTick ( softtimer_t* driver )
{
    if ( driver->state == ( uint8_t ) STS_RUNNING )
    {
        ++driver->counter;

        if ( driver->counter >= driver->period )
        {
            driver->expired = TRUE;
            driver->state = ( uint8_t ) STS_EXPIRED;
        }
        else
        {
            /* Intentionally blank */
        }
    }
    else
    {
        /* Intentionally blank */
    }
}

/**
 * @brief   Reports whether the timer has expired since the last call, and
 *          clears the flag.
 * @param[in,out] driver  Timer state. The flag this reports is cleared by the
 *                        call, so the parameter is genuinely in and out.
 * @return  TRUE when the timer expired since the previous call, FALSE
 *          otherwise.
 * @note    Reading consumes the event. Two calls in a row never both return
 *          TRUE for the same expiry.
 */
uint8_t softtimerExpired ( softtimer_t* driver )
{
    uint8_t retVal = FALSE;

    retVal = driver->expired;
    driver->expired = FALSE;

    return ( retVal );
}
```

`softtimerTick` sets `STS_EXPIRED` unconditionally at this point. Task 3 is where the periodic branch splits off — writing it now would be untested code.

- [ ] **Step 4: Compile and link to verify it passes**

Run both verification commands from Global Constraints. Expected: no warnings, link succeeds.

- [ ] **Step 5: Commit**

```bash
git add src/timer/softtimer.c test/SoftTimer_Test/SoftTimer_Test.c
git commit -m "+ One shot counting for the soft timer"
```

---

### Task 3: Periodic reload

The regression this module's test exists for.

**Files:**
- Modify: `src/timer/softtimer.c` (the `softtimerTick` body written in Task 2)
- Test: `test/SoftTimer_Test/SoftTimer_Test.c`

**Interfaces:**
- Consumes: everything from Tasks 1 and 2.
- Produces: no new symbols. `softtimerTick` gains the `STM_PERIODIC` branch.

- [ ] **Step 1: Write the failing test**

Insert into `test/SoftTimer_Test/SoftTimer_Test.c` above `main`:

```c
/* --------------------------------------------------------- periodic timing */

static void periodicCase ( void )
{
    softtimer_t driver;
    uint32_t i = 0;
    uint32_t expiries = 0;

    printf ( "periodic timing\n" );

    check ( "init", softtimerInit ( &driver, 10u, ( uint8_t ) STM_PERIODIC ) );

    softtimerStart ( &driver );

    for ( i = 0; i < 100u; ++i )
    {
        softtimerTick ( &driver );

        if ( softtimerExpired ( &driver ) == TRUE )
        {
            ++expiries;
        }
        else
        {
            /* Intentionally blank */
        }
    }

    printf ( "        expiries in 100 ticks at period 10: %lu\n",
             ( unsigned long ) expiries );

    check ( "a period of 10 expires 10 times in 100 ticks",
            ( uint8_t ) ( expiries == 10u ) );
    check ( "a periodic timer stays running rather than reaching STS_EXPIRED",
            ( uint8_t ) ( softtimerGetState ( &driver ) == ( uint8_t ) STS_RUNNING ) );
}

/*
 * Expiries that the main loop never reads must not disturb the tick the next
 * one lands on. Ticking 35 times without reading the flag leaves 5 counts of
 * the current period standing, so the next expiry is 5 ticks away, at tick 40.
 *
 * This pins the reload against a tick that forgets to reload at all, which
 * would leave the counter at 35 and never expire again. It does not pin the
 * subtracting reload against one that clears the counter — those two are the
 * same here, because the counter is always below the period on entry to a
 * tick and therefore lands exactly on it.
 */
static void periodicPhaseCase ( void )
{
    softtimer_t driver;
    uint32_t i = 0;

    printf ( "periodic phase after unread expiries\n" );

    check ( "init", softtimerInit ( &driver, 10u, ( uint8_t ) STM_PERIODIC ) );

    softtimerStart ( &driver );

    for ( i = 0; i < 35u; ++i )
    {
        softtimerTick ( &driver );
    }

    check ( "the unread expiries left the flag set",
            ( uint8_t ) ( softtimerExpired ( &driver ) == TRUE ) );
    check ( "5 ticks of the current period have already been counted",
            ( uint8_t ) ( softtimerGetElapsed ( &driver ) == 5u ) );

    for ( i = 0; i < 4u; ++i )
    {
        softtimerTick ( &driver );
    }

    check ( "tick 39 is still short of the next expiry",
            ( uint8_t ) ( softtimerExpired ( &driver ) == FALSE ) );

    softtimerTick ( &driver );

    check ( "the next expiry lands on tick 40, not tick 45",
            ( uint8_t ) ( softtimerExpired ( &driver ) == TRUE ) );
}
```

Add the calls to `main`, immediately after the existing `startStopCase ( );` line:

```c
    printf ( "\n" );
    periodicCase ( );
    printf ( "\n" );
    periodicPhaseCase ( );
```

`periodicPhaseCase` calls `softtimerGetElapsed`, which Task 4 implements. Implement `softtimerGetElapsed` as part of this task's Step 3 so the link closes — it is three lines and the phase check is not convincing without it.

- [ ] **Step 2: Link to verify it fails**

Run:

```bash
arm-none-eabi-gcc -Wall -Wextra -Iinc/timer --specs=nosys.specs \
  test/SoftTimer_Test/SoftTimer_Test.c src/timer/softtimer.c -o /tmp/softtimer_test.elf
```

Expected: FAIL, `undefined reference to 'softtimerGetElapsed'`.

Note what this red step does *not* prove. The periodic behaviour cases would link fine against the Task 2 code and fail only when executed — and nothing on this machine executes them. The link failure above is real but it is about the missing getter, not about the reload. Treat the reload as unverified until someone runs the binary on a host.

- [ ] **Step 3: Split the reload and add `softtimerGetElapsed`**

Replace the inner `if` of `softtimerTick` in `src/timer/softtimer.c`:

```c
        if ( driver->counter >= driver->period )
        {
            driver->expired = TRUE;
            driver->state = ( uint8_t ) STS_EXPIRED;
        }
```

with:

```c
        if ( driver->counter >= driver->period )
        {
            driver->expired = TRUE;

            if ( driver->mode == ( uint8_t ) STM_PERIODIC )
            {
                driver->counter -= driver->period;
            }
            else
            {
                driver->state = ( uint8_t ) STS_EXPIRED;
            }
        }
```

Add two new `@note` lines to the `softtimerTick` block, above the existing one shot note:

```c
 * @note    A missed expiry costs the event but not the phase: the reload
 *          happens here rather than at the read, so the next expiry lands on
 *          the tick it always would have. The number of missed expiries is
 *          not counted.
 * @note    The reload subtracts the period rather than clearing the counter.
 *          The counter is always below the period on entry, so it lands
 *          exactly on the period and the two are equivalent today. The
 *          subtraction is the form that stays correct if that ever changes.
```

Append `softtimerGetElapsed`:

```c
/**
 * @brief   Reports how many ticks the current period has counted.
 * @param[in] driver  Timer state.
 * @return  Ticks counted since the last start or the last periodic reload.
 */
uint32_t softtimerGetElapsed ( const softtimer_t* const driver )
{
    uint32_t retVal = 0u;

    retVal = driver->counter;

    return ( retVal );
}
```

- [ ] **Step 4: Compile and link to verify it passes**

Run both verification commands from Global Constraints. Expected: no warnings, link succeeds.

- [ ] **Step 5: Commit**

```bash
git add src/timer/softtimer.c test/SoftTimer_Test/SoftTimer_Test.c
git commit -m "+ Periodic reload that keeps its phase across unread expiries"
```

---

### Task 4: `softtimerGetRemaining` and `softtimerChangePeriod`

The last two functions and the module's second argument check.

**Files:**
- Modify: `src/timer/softtimer.c`
- Test: `test/SoftTimer_Test/SoftTimer_Test.c`

**Interfaces:**
- Consumes: everything from Tasks 1-3.
- Produces: `uint32_t softtimerGetRemaining ( const softtimer_t* const driver )`; `uint8_t softtimerChangePeriod ( softtimer_t* driver, uint32_t period )`.

- [ ] **Step 1: Write the failing test**

Insert into `test/SoftTimer_Test/SoftTimer_Test.c` above `main`:

```c
/* ------------------------------------------------- remaining and the period */

static void remainingCase ( void )
{
    softtimer_t driver;
    uint32_t i = 0;

    printf ( "elapsed and remaining\n" );

    check ( "init", softtimerInit ( &driver, 10u, ( uint8_t ) STM_ONESHOT ) );

    check ( "a fresh timer has its whole period remaining",
            ( uint8_t ) ( softtimerGetRemaining ( &driver ) == 10u ) );

    softtimerStart ( &driver );

    for ( i = 0; i < 4u; ++i )
    {
        softtimerTick ( &driver );
    }

    check ( "4 ticks elapsed", ( uint8_t ) ( softtimerGetElapsed ( &driver ) == 4u ) );
    check ( "6 ticks remaining", ( uint8_t ) ( softtimerGetRemaining ( &driver ) == 6u ) );

    softtimerStop ( &driver );

    for ( i = 0; i < 20u; ++i )
    {
        softtimerTick ( &driver );
    }

    check ( "stop froze the counter rather than clearing it",
            ( uint8_t ) ( softtimerGetElapsed ( &driver ) == 4u ) );

    softtimerStart ( &driver );

    for ( i = 0; i < 10u; ++i )
    {
        softtimerTick ( &driver );
    }

    check ( "remaining is zero once the timer has expired",
            ( uint8_t ) ( softtimerGetRemaining ( &driver ) == 0u ) );
}

static void changePeriodCase ( void )
{
    softtimer_t driver;
    uint32_t i = 0;

    printf ( "softtimerChangePeriod\n" );

    check ( "init", softtimerInit ( &driver, 10u, ( uint8_t ) STM_PERIODIC ) );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( softtimerChangePeriod ( NULL, 5u ) == FALSE ) );

    softtimerStart ( &driver );

    for ( i = 0; i < 7u; ++i )
    {
        softtimerTick ( &driver );
    }

    check ( "a zero period is rejected",
            ( uint8_t ) ( softtimerChangePeriod ( &driver, 0u ) == FALSE ) );
    check ( "the rejected period left the timer counting where it was",
            ( uint8_t ) ( softtimerGetElapsed ( &driver ) == 7u ) );
    check ( "and left the old period in place",
            ( uint8_t ) ( softtimerGetRemaining ( &driver ) == 3u ) );

    check ( "a change to 5 is accepted", softtimerChangePeriod ( &driver, 5u ) );
    check ( "the change reset the counter",
            ( uint8_t ) ( softtimerGetElapsed ( &driver ) == 0u ) );
    check ( "the timer is still running",
            ( uint8_t ) ( softtimerGetState ( &driver ) == ( uint8_t ) STS_RUNNING ) );

    for ( i = 0; i < 4u; ++i )
    {
        softtimerTick ( &driver );
    }

    check ( "4 ticks is short of the new period",
            ( uint8_t ) ( softtimerExpired ( &driver ) == FALSE ) );

    softtimerTick ( &driver );

    check ( "and the 5th tick expires it",
            ( uint8_t ) ( softtimerExpired ( &driver ) == TRUE ) );
}
```

Add the calls to `main`, immediately after the existing `periodicPhaseCase ( );` line:

```c
    printf ( "\n" );
    remainingCase ( );
    printf ( "\n" );
    changePeriodCase ( );
```

- [ ] **Step 2: Link to verify it fails**

Run the link command. Expected: FAIL, `undefined reference to 'softtimerGetRemaining'` and `'softtimerChangePeriod'`.

- [ ] **Step 3: Implement both functions**

Append to `src/timer/softtimer.c`:

```c
/**
 * @brief   Reports how many ticks are left before the timer expires.
 * @param[in] driver  Timer state.
 * @return  Ticks remaining in the current period, or zero once the counter has
 *          reached it.
 * @note    The comparison is explicit rather than an unsigned subtraction,
 *          which would wrap for an expired one shot.
 */
uint32_t softtimerGetRemaining ( const softtimer_t* const driver )
{
    uint32_t retVal = 0u;

    if ( driver->counter < driver->period )
    {
        retVal = driver->period - driver->counter;
    }
    else
    {
        retVal = 0u;
    }

    return ( retVal );
}

/**
 * @brief   Installs a new period and restarts the current interval.
 * @param[in,out] driver  Timer state.
 * @param[in]     period  New number of ticks per timeout.
 * @return  TRUE on success, FALSE when driver is NULL or period is zero.
 *          Nothing is written to the driver when FALSE is returned.
 * @note    This returns a status because it takes a new argument that would
 *          break softtimerTick, which is the same reason
 *          pidChangeCoefficients returns one.
 * @note    The counter is cleared. Without that, a new period smaller than the
 *          current count would expire on consecutive ticks until the counter
 *          drained. The state and the expiry flag are left alone, so a
 *          finished one shot stays finished until it is started again.
 */
uint8_t softtimerChangePeriod ( softtimer_t* driver, uint32_t period )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( period > 0u ) )
    {
        driver->period = period;
        driver->counter = 0u;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}
```

- [ ] **Step 4: Compile and link to verify it passes**

Run both verification commands from Global Constraints. Expected: no warnings, link succeeds. All nine exported functions are now defined.

- [ ] **Step 5: Commit**

```bash
git add src/timer/softtimer.c test/SoftTimer_Test/SoftTimer_Test.c
git commit -m "+ Remaining time and period change for the soft timer"
```

---

### Task 5: Tree-wide verification and CLAUDE.md

The module is written; this task proves it did not break anything and records it where the next reader looks.

**Files:**
- Modify: `CLAUDE.md`

**Interfaces:**
- Consumes: the finished module from Tasks 1-4.
- Produces: nothing the code depends on.

- [ ] **Step 1: Check the whole tree still compiles clean**

Run:

```bash
for f in src/*/*.c drv/*.c; do m=$(basename $(dirname "$f")); inc="inc/$m"; [ -d "$inc" ] || inc="drv"; \
  arm-none-eabi-gcc -c -Wall -Wextra -I"$inc" -Idrv "$f" -o /dev/null; done
```

Expected: no output at all. Any warning is a regression.

- [ ] **Step 2: Check every header still coexists in one translation unit**

This is what catches a duplicate include guard or a clashing typedef, and `softtimer.h` is a new guard and a new struct.

```bash
for h in inc/*/*.h drv/*.h; do echo "#include \"$(basename $h)\""; done > /tmp/allhdr.c
echo "int main(void){return 0;}" >> /tmp/allhdr.c
arm-none-eabi-gcc -c -Wall $(for d in inc/*/ drv/; do echo -n " -I$d"; done) /tmp/allhdr.c -o /dev/null
```

Expected: no output. In particular no redefinition of `STM_ONESHOT` or `STS_STOPPED` against any existing enum member.

- [ ] **Step 3: Check the exported symbols are all prefixed and all tested**

```bash
mkdir -p /tmp/objs
arm-none-eabi-gcc -c -Wall -Iinc/timer src/timer/softtimer.c -o /tmp/objs/softtimer.o
arm-none-eabi-nm /tmp/objs/softtimer.o | grep ' T ' | awk '{print $3}' | sort -u
```

Expected, exactly nine lines and every one starting with `softtimer`:

```
softtimerChangePeriod
softtimerExpired
softtimerGetElapsed
softtimerGetRemaining
softtimerGetState
softtimerInit
softtimerStart
softtimerStop
softtimerTick
```

Then confirm each is referenced by the test:

```bash
arm-none-eabi-nm /tmp/objs/softtimer.o | grep ' T ' | awk '{print $3}' | sort -u | \
  while read s; do grep -q "\b$s\b" test/SoftTimer_Test/SoftTimer_Test.c || echo "UNCALLED: $s"; done
```

Expected: no output.

- [ ] **Step 4: Update CLAUDE.md**

Three edits, all factual:

1. In the "What this repo is" opening paragraph, add the module to the list of what the library contains. The current sentence begins "filters, PID/hysteresis control, circular buffer, CRC, sort/search, matrix/complex math, serial protocol handlers, and shift-register drivers" — insert `soft timers` into that list.

2. Add a short paragraph after the `sort`/`search` paragraph:

```markdown
`softtimer` is the library's only time abstraction. It counts calls to `softtimerTick`, which the caller makes from a fixed-rate ISR, so the period is expressed in ticks and the interrupt rate is the unit — the same rule `hc595Interrupt` follows. One-shot and periodic modes share one struct; a periodic reload subtracts the period rather than clearing the counter, so an expiry the main loop fails to read costs the event but never the phase. It is deliberately not consumed by `comat`, `dcMotor` or the shift-register drivers: they keep their own counters, because module independence forbids one module including another's header.
```

3. In the "Testing" section, change the test count from twenty to twenty-one, the exported symbol count from 181 to 190, add `SoftTimer_Test` to the list of assert-style tests, and add a row to the table of tests that pin a specific bug:

```markdown
| `SoftTimer_Test` | a periodic timer stopping at its first expiry instead of reloading, and a one-shot that keeps counting after it has expired |
```

- [ ] **Step 5: Commit**

```bash
git add CLAUDE.md
git commit -m "+ Record the soft timer module in the repository guide"
```

- [ ] **Step 6: Report honestly what was verified**

The final report must say: the module and its test compile clean under `-Wall -Wextra` and link, all nine symbols are exported with the module prefix and referenced by the test, and the whole tree plus the all-headers translation unit still build without warnings. It must also say that **no test was executed**, because this machine has no host compiler, and that the numeric expectations in `SoftTimer_Test.c` — particularly the tick-40 phase check — are therefore unverified until someone runs:

```bash
gcc -Wall -Wextra -Iinc/timer test/SoftTimer_Test/SoftTimer_Test.c src/timer/softtimer.c -o softtimer_test && ./softtimer_test
```

Do not describe the tests as passing.
