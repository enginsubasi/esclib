# comstxetx Transparency and Integrity Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `comstxetx` carry arbitrary binary payloads and reject frames corrupted in transit, by adding DLE escaping, a caller-supplied checksum, and a matching frame builder.

**Architecture:** `inc/communication/comstxetx.h` and `src/communication/comstxetx.c` change in place. The receive state machine gains an escape state and a frame-open flag; verification moves into `comstxetxEvaluate` because a checksum is O(n) and `comstxetxReceive` runs per byte from an interrupt. A new `comstxetxBuildFrame` fills the transmit buffer the module already owns but never used, so the encoder and the decoder live in one file and cannot drift apart.

**Tech Stack:** Freestanding C89-style C, `<stdint.h>` types, `<stddef.h>` for `NULL`. No heap, no OS, no build system.

Spec: `docs/superpowers/specs/2026-08-05-comstxetx-integrity-design.md`

## Global Constraints

Every task's requirements implicitly include this section.

- **This is a breaking change and that is intended.** `comstxetxInit` gains two parameters and `packetProcess` receives different indices. Existing callers must not compile. Do not add a compatibility shim.
- **`comat` is out of scope.** Do not add integrity checking to it. It implements AT commands, whose real peers carry no checksum. The only change `comat.c` may receive in this plan is the one banner sentence in Task 4.
- Zero warnings under `-Wall -Wextra`. A new warning is a regression.
- `src/communication/comstxetx.c` includes only `<stddef.h>` and `"comstxetx.h"`. No other module's header, ever — module independence is what makes single-module copy-out work. The checksum arrives as a function pointer precisely because `comstxetx.c` may not include `crc16.h`.
- Style, enforced exactly (`codingReference.md`):
  - Spaces inside every paren: `if ( ( a > b ) || ( c == d ) )`, `foo ( &driver, 5 )`.
  - Allman braces, braces on every block including single statements.
  - Pre-increment: `++driver->rxIndex`.
  - One `retVal` local, initialized at its declaration, single exit written `return ( retVal );`.
  - Status returns use `TRUE`/`FALSE`, never `0`/`1` literals.
  - Empty `else` branches written out with `/* Intentionally blank */`, never omitted.
  - `NULL` from `<stddef.h>`, never a bare `0`.
  - At most one `break` in a loop; prefer a flag in the loop condition.
  - All locals declared at the top of the function, C89 style.
- Validation happens in `comstxetxInit` and `comstxetxBuildFrame` only. `comstxetxReceive`, `comstxetxEvaluate`, `comstxetxTimeoutCounter` and the accessor dereference `driver` unchecked — they are the interrupt and main-loop hot paths.
- Documentation lives in the `.c` only. The header is a pure declaration file with all template sections preserved, including the empty ones.
- Commit messages are terse and prefixed: `+` for additions, `*` for fixes. **Never** add a `Co-Authored-By:` trailer, a "Generated with Claude Code" footer, or any other AI attribution.

### Verification commands

A host compiler is installed but is **not** on PATH. Prepend it before running anything:

```bash
export PATH="$LOCALAPPDATA/Microsoft/WinGet/Packages/BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe/mingw64/bin:$PATH"
```

Build and run the protocol test — this is the real gate, and unlike earlier work in this repository it can be executed:

```bash
gcc -Wall -Wextra -Iinc/communication -Iinc/crc \
  test/Protocol_Test/Protocol_Test.c src/communication/comat.c src/communication/comstxetx.c src/crc/crc16.c \
  -o /tmp/protocol_test && /tmp/protocol_test
```

Exit status 0 and a final line of `all checks passed` is the pass condition. Any `FAIL` line is a failure.

Warning check with the cross compiler, which is on PATH:

```bash
arm-none-eabi-gcc -c -Wall -Wextra -Iinc/communication src/communication/comstxetx.c -o /dev/null
```

## File Structure

| File | Responsibility |
|---|---|
| `inc/communication/comstxetx.h` | Modify. Struct gains `dle`, `rxFrameOpen`, `rxEscape`, `rxRejectCount` and the checksum pointer; two new prototypes; `volatile` on the shared receive state. |
| `src/communication/comstxetx.c` | Modify. `Init` validation, the escaping receive path, checksum verification in `Evaluate`, the timeout key change, and the new `BuildFrame` and accessor. |
| `test/Protocol_Test/Protocol_Test.c` | Modify. The `comat` cases are untouched; the `comstxetx` half is rewritten for the new frame format and grows the round-trip case. |
| `src/communication/comat.c` | Modify, Task 4 only. One banner sentence saying why it carries no checksum. |
| `CLAUDE.md` | Modify, Task 4 only. Symbol count, module description, pinned-bug table row. |

---

### Task 1: Header and the Init contract

The struct and the argument checks. The receive path still behaves as it does today, so the existing tests keep passing once their `Init` call is updated.

**Files:**
- Modify: `inc/communication/comstxetx.h`
- Modify: `src/communication/comstxetx.c`
- Test: `test/Protocol_Test/Protocol_Test.c`

**Interfaces:**
- Consumes: nothing from earlier tasks.
- Produces: the `comstxetx_t` layout below; `uint8_t comstxetxInit ( comstxetx_t* driver, uint8_t* rxBuffer, uint8_t* txBuffer, uint32_t rxSize, uint32_t txSize, uint8_t stx, uint8_t etx, uint8_t dle, uint32_t rxTimeout, uint16_t ( *checksum ) ( const uint8_t* const buffer, uint32_t length ), void ( *packetProcess ) ( uint8_t* buffer, uint32_t index ) )`.

- [ ] **Step 1: Replace the struct and prototypes in the header**

In `inc/communication/comstxetx.h`, replace the `comstxetx_t` definition with:

```c
typedef struct
{
    uint8_t stx;
    uint8_t etx;
    uint8_t dle;

    volatile uint32_t rxTimeoutCounter;
    uint32_t rxTimeout;

    volatile uint32_t rxIndex;
    uint32_t rxSize;
    uint32_t txSize;

    /*
     * Shared between comstxetxReceive on the interrupt side and the caller
     * side. Declared volatile so the writes stay ordered with respect to each
     * other and neither side caches state in a register.
     */
    volatile uint8_t rxFrameOpen;
    volatile uint8_t rxEscape;
    volatile uint8_t rxReadyToEvaluate;

    uint32_t rxRejectCount;

    uint8_t *rxBuffer;

    uint8_t *txBuffer;

    uint16_t ( *checksum ) ( const uint8_t* const buffer, uint32_t length );
    void ( *packetProcess ) ( uint8_t* buffer, uint32_t index );
} comstxetx_t;
```

`rxRejectCount` is deliberately not volatile: it is written by `comstxetxEvaluate` and read by the accessor, both on the caller side.

Replace the prototype block with:

```c
uint8_t comstxetxInit ( comstxetx_t* driver, uint8_t* rxBuffer, uint8_t* txBuffer,
                        uint32_t rxSize, uint32_t txSize,
                        uint8_t stx, uint8_t etx, uint8_t dle,
                        uint32_t rxTimeout,
                        uint16_t ( *checksum ) ( const uint8_t* const buffer, uint32_t length ),
                        void ( *packetProcess ) ( uint8_t* buffer, uint32_t index ) );
void comstxetxReceive ( comstxetx_t* driver, uint8_t data );
void comstxetxEvaluate ( comstxetx_t* driver );
void comstxetxTimeoutCounter ( comstxetx_t* driver );
uint8_t comstxetxBuildFrame ( comstxetx_t* driver, const uint8_t* const payload,
                              uint32_t length, uint32_t* frameLength );
uint32_t comstxetxGetRejectCount ( const comstxetx_t* const driver );
```

`comstxetxBuildFrame` and `comstxetxGetRejectCount` are declared now and defined in Tasks 3 and 2. Nothing calls them until then, so the link stays closed.

- [ ] **Step 2: Write the failing test**

In `test/Protocol_Test/Protocol_Test.c`, add a checksum probe next to the other `comstxetx` probes (after `sxFeed`), and add `#include "crc16.h"` beside the existing includes at the top of the file:

```c
/*
 * A byte sum rather than a real CRC, so every expected value in this file can
 * be worked out by hand. crc16 is exercised separately in sxCrcCase to prove
 * the callback type takes it with no wrapper.
 */
static uint16_t sxSumChecksum ( const uint8_t* const buffer, uint32_t length )
{
    uint16_t retVal = 0;
    uint32_t i = 0;

    for ( i = 0; i < length; ++i )
    {
        retVal = ( uint16_t ) ( retVal + buffer[ i ] );
    }

    return ( retVal );
}
```

Then add this case function above `main`:

```c
static void sxInitCase ( void )
{
    comstxetx_t driver;
    uint8_t rxBuffer[ 32 ];
    uint8_t txBuffer[ 32 ];

    printf ( "comstxetx init contract\n" );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( comstxetxInit ( NULL, rxBuffer, txBuffer, 32u, 32u,
                                          0x02u, 0x03u, 0x10u, 10u,
                                          sxSumChecksum, sxPacketProcess ) == FALSE ) );

    check ( "stx equal to etx is rejected",
            ( uint8_t ) ( comstxetxInit ( &driver, rxBuffer, txBuffer, 32u, 32u,
                                          0x02u, 0x02u, 0x10u, 10u,
                                          sxSumChecksum, sxPacketProcess ) == FALSE ) );

    check ( "dle equal to stx is rejected",
            ( uint8_t ) ( comstxetxInit ( &driver, rxBuffer, txBuffer, 32u, 32u,
                                          0x02u, 0x03u, 0x02u, 10u,
                                          sxSumChecksum, sxPacketProcess ) == FALSE ) );

    check ( "dle equal to etx is rejected",
            ( uint8_t ) ( comstxetxInit ( &driver, rxBuffer, txBuffer, 32u, 32u,
                                          0x02u, 0x03u, 0x03u, 10u,
                                          sxSumChecksum, sxPacketProcess ) == FALSE ) );

    check ( "a NULL checksum is rejected",
            ( uint8_t ) ( comstxetxInit ( &driver, rxBuffer, txBuffer, 32u, 32u,
                                          0x02u, 0x03u, 0x10u, 10u,
                                          NULL, sxPacketProcess ) == FALSE ) );

    check ( "a NULL packetProcess is rejected",
            ( uint8_t ) ( comstxetxInit ( &driver, rxBuffer, txBuffer, 32u, 32u,
                                          0x02u, 0x03u, 0x10u, 10u,
                                          sxSumChecksum, NULL ) == FALSE ) );

    check ( "an rxSize below two is rejected",
            ( uint8_t ) ( comstxetxInit ( &driver, rxBuffer, txBuffer, 1u, 32u,
                                          0x02u, 0x03u, 0x10u, 10u,
                                          sxSumChecksum, sxPacketProcess ) == FALSE ) );

    check ( "a txSize below four is rejected",
            ( uint8_t ) ( comstxetxInit ( &driver, rxBuffer, txBuffer, 32u, 3u,
                                          0x02u, 0x03u, 0x10u, 10u,
                                          sxSumChecksum, sxPacketProcess ) == FALSE ) );

    check ( "a well formed init succeeds",
            comstxetxInit ( &driver, rxBuffer, txBuffer, 32u, 32u,
                            0x02u, 0x03u, 0x10u, 10u,
                            sxSumChecksum, sxPacketProcess ) );

    check ( "init leaves no frame open",
            ( uint8_t ) ( driver.rxFrameOpen == FALSE ) );
    check ( "init leaves no escape pending",
            ( uint8_t ) ( driver.rxEscape == FALSE ) );
    check ( "init clears the reject count",
            ( uint8_t ) ( comstxetxGetRejectCount ( &driver ) == 0u ) );
}
```

Add its call in `main` immediately before the existing `comstxetxCase ( );` line:

```c
    sxInitCase ( );
    printf ( "\n" );
```

- [ ] **Step 3: Run the test to verify it fails**

```bash
export PATH="$LOCALAPPDATA/Microsoft/WinGet/Packages/BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe/mingw64/bin:$PATH"
gcc -Wall -Wextra -Iinc/communication -Iinc/crc \
  test/Protocol_Test/Protocol_Test.c src/communication/comat.c src/communication/comstxetx.c src/crc/crc16.c \
  -o /tmp/protocol_test
```

Expected: FAIL to build. The existing `comstxetxInit` definition in the `.c` no longer matches the header prototype, and `comstxetxGetRejectCount` is undefined. Both are fixed in Step 4.

- [ ] **Step 4: Update Init and add the accessor**

In `src/communication/comstxetx.c`, replace the `COMSTXETX_MIN_RX_SIZE` define block with:

```c
// comstxetxReceive writes rxBuffer[ rxIndex ] and only afterwards checks the
// index against rxSize. The smallest meaningful frame carries an empty payload
// and the two checksum bytes, so that pair is also the smallest buffer that
// never takes a write past its end.
#define COMSTXETX_MIN_RX_SIZE   2

// The smallest frame comstxetxBuildFrame can emit is STX, two checksum bytes
// and ETX, with no byte needing an escape.
#define COMSTXETX_MIN_TX_SIZE   4

// Bytes of checksum carried at the end of every frame, low byte first.
#define COMSTXETX_CHECKSUM_SIZE 2
```

Replace the whole `comstxetxInit` function, documentation block included, with:

```c
/**
 * @brief   Initializes the STX, ETX communication framework.
 * @param[out] driver         Framework state to initialize.
 * @param[in]  rxBuffer       Caller owned receive buffer.
 * @param[in]  txBuffer       Caller owned transmit buffer.
 * @param[in]  rxSize         Size of rxBuffer in bytes.
 * @param[in]  txSize         Size of txBuffer in bytes.
 * @param[in]  stx            Byte that marks the start of a frame.
 * @param[in]  etx            Byte that marks the end of a frame.
 * @param[in]  dle            Escape byte. A payload byte equal to stx, etx or
 *                            dle is sent preceded by this byte, and the byte
 *                            after it is always data.
 * @param[in]  rxTimeout      Number of comstxetxTimeoutCounter ticks a
 *                            partial frame may stay pending. It is discarded
 *                            once the tick count exceeds this value, so the
 *                            budget is rxTimeout + 1 ticks. This is a whole
 *                            frame timeout, not an inter-byte one.
 * @param[in]  checksum       Computes the frame check over the unescaped
 *                            payload. The signature is that of crc16, so
 *                            crc16 and crc16Alt can be passed directly.
 * @param[in]  packetProcess  Called with the payload alone, without the STX
 *                            byte and without the checksum bytes.
 * @return  TRUE on success, FALSE when a pointer is NULL, rxSize is below
 *          two, txSize is below four, or stx, etx and dle are not three
 *          distinct values.
 * @note    Both buffers are zero filled here and are not copied. They must
 *          outlive the driver.
 * @note    Both callbacks are required. comstxetxEvaluate calls them without
 *          checking, so a NULL here would only surface as a crash on the
 *          first complete frame.
 * @note    The three framing bytes must differ from one another. Two equal
 *          values leave at least one of them unable to mean what it names.
 */
uint8_t comstxetxInit ( comstxetx_t* driver, uint8_t* rxBuffer, uint8_t* txBuffer,
                        uint32_t rxSize, uint32_t txSize,
                        uint8_t stx, uint8_t etx, uint8_t dle,
                        uint32_t rxTimeout,
                        uint16_t ( *checksum ) ( const uint8_t* const buffer, uint32_t length ),
                        void ( *packetProcess ) ( uint8_t* buffer, uint32_t index ) )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;

    if ( ( driver != NULL ) && ( rxBuffer != NULL ) && ( txBuffer != NULL ) &&
            ( rxSize >= COMSTXETX_MIN_RX_SIZE ) && ( txSize >= COMSTXETX_MIN_TX_SIZE ) &&
            ( stx != etx ) && ( stx != dle ) && ( etx != dle ) &&
            ( checksum != NULL ) && ( packetProcess != NULL ) )
    {
        // Function assignment.
        driver->checksum = checksum;
        driver->packetProcess = packetProcess;

        // Parameter settings.
        driver->rxBuffer = rxBuffer;
        driver->txBuffer = txBuffer;

        driver->rxSize = rxSize;
        driver->txSize = txSize;

        driver->stx = stx;
        driver->etx = etx;
        driver->dle = dle;

        driver->rxTimeoutCounter = 0;
        driver->rxTimeout = rxTimeout;

        // Initialize to zero and FALSE
        driver->rxIndex = 0;
        driver->rxFrameOpen = FALSE;
        driver->rxEscape = FALSE;
        driver->rxReadyToEvaluate = FALSE;
        driver->rxRejectCount = 0;

        // Fill with zero
        for ( i = 0; i < driver->rxSize; ++i )
        {
            driver->rxBuffer[ i ] = 0;
        }

        for ( i = 0; i < driver->txSize; ++i )
        {
            driver->txBuffer[ i ] = 0;
        }

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}
```

Append the accessor at the end of the file:

```c
/**
 * @brief   Reports how many frames have been discarded without reaching
 *          packetProcess.
 * @param[in] driver  Framework state.
 * @return  Count of frames rejected since Init.
 * @note    It counts both causes together: a frame whose checksum did not
 *          match, and a frame too short to carry a checksum at all. Nothing a
 *          caller does differs between the two, so they are not separated.
 */
uint32_t comstxetxGetRejectCount ( const comstxetx_t* const driver )
{
    uint32_t retVal = 0;

    retVal = driver->rxRejectCount;

    return ( retVal );
}
```

- [ ] **Step 5: Update the existing comstxetxCase Init call**

`comstxetxCase` already exists in the test file and calls `comstxetxInit` with the old argument list. Add the `dle` byte and the checksum argument so it compiles. Choose `0x10u` for `dle`, and pass `sxSumChecksum`. Do **not** change its expectations yet — the receive path is unchanged in this task, so its existing assertions about STX at index 0 still hold.

- [ ] **Step 6: Run the test to verify it passes**

Run the build-and-run command from Step 3. Expected: exit status 0, final line `all checks passed`, and no compiler warnings.

Also run the cross-compiler warning check:

```bash
arm-none-eabi-gcc -c -Wall -Wextra -Iinc/communication src/communication/comstxetx.c -o /dev/null
```

Expected: no output.

- [ ] **Step 7: Commit**

```bash
git add inc/communication/comstxetx.h src/communication/comstxetx.c test/Protocol_Test/Protocol_Test.c
git commit -m "+ Escape byte, checksum hook and reject counter in the comstxetx contract"
```

---

### Task 2: The receive path

Escaping, payload-only storage, resynchronisation, the timeout key change, and checksum verification. The frame format changes exactly once, here.

**Files:**
- Modify: `src/communication/comstxetx.c`
- Test: `test/Protocol_Test/Protocol_Test.c`

**Interfaces:**
- Consumes: the struct and `comstxetxInit` from Task 1; `COMSTXETX_CHECKSUM_SIZE`.
- Produces: no new symbols. `comstxetxReceive`, `comstxetxEvaluate` and `comstxetxTimeoutCounter` change behaviour.

- [ ] **Step 1: Write the failing tests**

Replace the whole existing `comstxetxCase` function with the four case functions below, and add a byte-array feeder next to `sxFeed`:

```c
static void sxFeedBytes ( comstxetx_t* driver, const uint8_t* const bytes, uint32_t length )
{
    uint32_t i = 0;

    for ( i = 0; i < length; ++i )
    {
        comstxetxReceive ( driver, bytes[ i ] );
    }
}
```

`sxFeed`, which walks a NUL terminated string, stays for any text frame but cannot express a binary payload — a zero byte would end the string and an escape byte is not printable.

```c
/*
 * Framing with stx 0x02, etx 0x03, dle 0x10.
 *
 * The payload is 0x41 0x03 0x10, chosen because the second byte is ETX and the
 * third is DLE. Without escaping the frame would end at the second byte, which
 * is the hole this work exists to close.
 *
 * Sum checksum over { 0x41, 0x03, 0x10 } is 0x54, so the frame carries 0x54
 * then 0x00, low byte first. Neither collides with a framing byte, so neither
 * needs an escape.
 */
static void sxEscapeCase ( void )
{
    comstxetx_t driver;
    uint8_t rxBuffer[ 32 ];
    uint8_t txBuffer[ 32 ];
    static const uint8_t wire[ ] =
    {
        0x02u,                  /* STX                        */
        0x41u,                  /* 'A', no escape needed      */
        0x10u, 0x03u,           /* escaped ETX in the payload */
        0x10u, 0x10u,           /* escaped DLE in the payload */
        0x54u, 0x00u,           /* checksum, low byte first   */
        0x03u                   /* ETX                        */
    };

    printf ( "comstxetx escaping\n" );

    sxReset ( );

    check ( "init", comstxetxInit ( &driver, rxBuffer, txBuffer, 32u, 32u,
                                    0x02u, 0x03u, 0x10u, 10u,
                                    sxSumChecksum, sxPacketProcess ) );

    sxFeedBytes ( &driver, wire, ( uint32_t ) sizeof ( wire ) );
    comstxetxEvaluate ( &driver );

    check ( "the frame reached packetProcess", ( uint8_t ) ( sxProcessCalls == 1u ) );
    check ( "the payload is three bytes, checksum stripped",
            ( uint8_t ) ( sxLastLength == 3u ) );
    check ( "the escaped ETX arrived as data",
            ( uint8_t ) ( sxLastFrame[ 1 ] == 0x03u ) );
    check ( "the escaped DLE arrived as data",
            ( uint8_t ) ( sxLastFrame[ 2 ] == 0x10u ) );
    check ( "the unescaped byte is unchanged",
            ( uint8_t ) ( sxLastFrame[ 0 ] == 0x41u ) );
    check ( "nothing was rejected",
            ( uint8_t ) ( comstxetxGetRejectCount ( &driver ) == 0u ) );
}

/*
 * A single corrupted payload byte must be rejected rather than delivered, and
 * a frame that closes with fewer than two stored bytes cannot carry a checksum
 * at all.
 */
static void sxRejectCase ( void )
{
    comstxetx_t driver;
    uint8_t rxBuffer[ 32 ];
    uint8_t txBuffer[ 32 ];
    static const uint8_t corrupt[ ] =
    {
        /*
         * A valid single byte frame carrying 0x41 would read
         * 0x02 0x41 0x41 0x00 0x03, the check being the sum of one byte. Here
         * the payload byte alone was flipped to 0x42, so the check still
         * claims 0x41 while the payload now sums to 0x42.
         */
        0x02u, 0x42u, 0x41u, 0x00u, 0x03u
    };
    static const uint8_t tooShort[ ] =
    {
        0x02u, 0x41u, 0x03u                 /* one stored byte, then ETX      */
    };

    printf ( "comstxetx rejection\n" );

    sxReset ( );

    check ( "init", comstxetxInit ( &driver, rxBuffer, txBuffer, 32u, 32u,
                                    0x02u, 0x03u, 0x10u, 10u,
                                    sxSumChecksum, sxPacketProcess ) );

    sxFeedBytes ( &driver, corrupt, ( uint32_t ) sizeof ( corrupt ) );
    comstxetxEvaluate ( &driver );

    check ( "a corrupted frame does not reach packetProcess",
            ( uint8_t ) ( sxProcessCalls == 0u ) );
    check ( "and is counted",
            ( uint8_t ) ( comstxetxGetRejectCount ( &driver ) == 1u ) );

    sxFeedBytes ( &driver, tooShort, ( uint32_t ) sizeof ( tooShort ) );
    comstxetxEvaluate ( &driver );

    check ( "a frame too short to carry a checksum does not reach packetProcess",
            ( uint8_t ) ( sxProcessCalls == 0u ) );
    check ( "and is counted too",
            ( uint8_t ) ( comstxetxGetRejectCount ( &driver ) == 2u ) );
}

/*
 * An unescaped STX inside an open frame can only mean the sender restarted,
 * because a payload STX always arrives escaped. The bytes before it are
 * abandoned and the frame begins again.
 */
static void sxResyncCase ( void )
{
    comstxetx_t driver;
    uint8_t rxBuffer[ 32 ];
    uint8_t txBuffer[ 32 ];
    static const uint8_t wire[ ] =
    {
        0x02u, 0x99u, 0x99u,    /* abandoned by the STX that follows */
        0x02u,                  /* restart                           */
        0x41u, 0x41u, 0x00u,    /* payload 0x41, checksum 0x41 0x00  */
        0x03u
    };

    printf ( "comstxetx resynchronisation\n" );

    sxReset ( );

    check ( "init", comstxetxInit ( &driver, rxBuffer, txBuffer, 32u, 32u,
                                    0x02u, 0x03u, 0x10u, 10u,
                                    sxSumChecksum, sxPacketProcess ) );

    sxFeedBytes ( &driver, wire, ( uint32_t ) sizeof ( wire ) );
    comstxetxEvaluate ( &driver );

    check ( "the restarted frame reached packetProcess",
            ( uint8_t ) ( sxProcessCalls == 1u ) );
    check ( "only the bytes after the second STX are payload",
            ( uint8_t ) ( sxLastLength == 1u ) );
    check ( "and the payload is the byte that followed it",
            ( uint8_t ) ( sxLastFrame[ 0 ] == 0x41u ) );
}

/*
 * The pinned regression.
 *
 * With STX no longer stored, rxIndex == 0 no longer means no frame is open. A
 * frame that has opened on STX but received no payload byte yet must still
 * time out. Keying comstxetxTimeoutCounter off rxIndex leaves it pending for
 * good, and every other case in this file passes with that bug present.
 */
static void sxOpenEmptyTimeoutCase ( void )
{
    comstxetx_t driver;
    uint8_t rxBuffer[ 32 ];
    uint8_t txBuffer[ 32 ];
    uint32_t i = 0;
    static const uint8_t rest[ ] = { 0x41u, 0x41u, 0x00u, 0x03u };

    printf ( "comstxetx timeout on an opened but empty frame\n" );

    sxReset ( );

    check ( "init", comstxetxInit ( &driver, rxBuffer, txBuffer, 32u, 32u,
                                    0x02u, 0x03u, 0x10u, 3u,
                                    sxSumChecksum, sxPacketProcess ) );

    comstxetxReceive ( &driver, 0x02u );

    check ( "the frame is open", ( uint8_t ) ( driver.rxFrameOpen == TRUE ) );

    for ( i = 0; i < 5u; ++i )
    {
        comstxetxTimeoutCounter ( &driver );
    }

    check ( "the empty open frame timed out",
            ( uint8_t ) ( driver.rxFrameOpen == FALSE ) );

    /*
     * The bytes that would have completed the abandoned frame must not be
     * taken as a frame of their own, because no STX opened them.
     */
    sxFeedBytes ( &driver, rest, ( uint32_t ) sizeof ( rest ) );
    comstxetxEvaluate ( &driver );

    check ( "the orphaned tail did not become a frame",
            ( uint8_t ) ( sxProcessCalls == 0u ) );
}
```

```c
/*
 * An escape sequence straddling the end of the buffer must not write past it.
 * rxSize is deliberately smaller than the array, and the byte just beyond it
 * carries a sentinel that Init did not touch, so an overrun is visible.
 */
static void sxBoundaryCase ( void )
{
    comstxetx_t driver;
    uint8_t rxBuffer[ 8 ];
    uint8_t txBuffer[ 32 ];
    static const uint8_t wire[ ] =
    {
        0x02u,                  /* STX                          */
        0x10u, 0x02u,           /* escaped STX, stores one byte */
        0x11u,                  /* stores one byte              */
        0x12u,                  /* stores one byte              */
        0x10u, 0x03u            /* escaped ETX, the fourth store fills rxSize */
    };

    printf ( "comstxetx escape at the buffer boundary\n" );

    sxReset ( );

    check ( "init with an rxSize of four",
            comstxetxInit ( &driver, rxBuffer, txBuffer, 4u, 32u,
                            0x02u, 0x03u, 0x10u, 10u,
                            sxSumChecksum, sxPacketProcess ) );

    rxBuffer[ 4 ] = 0xEEu;

    sxFeedBytes ( &driver, wire, ( uint32_t ) sizeof ( wire ) );

    check ( "the byte past rxSize is untouched",
            ( uint8_t ) ( rxBuffer[ 4 ] == 0xEEu ) );
    check ( "the overrun frame was discarded",
            ( uint8_t ) ( driver.rxFrameOpen == FALSE ) );
    check ( "and no escape stayed pending",
            ( uint8_t ) ( driver.rxEscape == FALSE ) );

    comstxetxEvaluate ( &driver );

    check ( "nothing reached packetProcess",
            ( uint8_t ) ( sxProcessCalls == 0u ) );
}
```

In `main`, replace the `comstxetxCase ( );` call with:

```c
    sxEscapeCase ( );
    printf ( "\n" );
    sxRejectCase ( );
    printf ( "\n" );
    sxResyncCase ( );
    printf ( "\n" );
    sxBoundaryCase ( );
    printf ( "\n" );
    sxOpenEmptyTimeoutCase ( );
```

- [ ] **Step 2: Run the tests to verify they fail**

```bash
export PATH="$LOCALAPPDATA/Microsoft/WinGet/Packages/BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe/mingw64/bin:$PATH"
gcc -Wall -Wextra -Iinc/communication -Iinc/crc \
  test/Protocol_Test/Protocol_Test.c src/communication/comat.c src/communication/comstxetx.c src/crc/crc16.c \
  -o /tmp/protocol_test && /tmp/protocol_test
```

Expected: it builds and runs, and the `comstxetx` cases FAIL. The old receive path stores STX at index 0, never unescapes, and delivers the checksum bytes as payload, so the length and content checks all miss.

- [ ] **Step 3: Replace the receive path**

In `src/communication/comstxetx.c`, replace `comstxetxReceive`, documentation block included, with:

```c
/**
 * @brief   Called from the receive interrupt, one byte at a time; assembles
 *          the unescaped payload of a frame that starts at the STX byte and
 *          completes at an unescaped ETX byte.
 * @param[in,out] driver  Framework state.
 * @param[in]     data    Byte received from the interface.
 * @note    The buffer holds the payload alone. STX is not stored, ETX is not
 *          stored, and the escape bytes are consumed as they are seen. The
 *          two checksum bytes are stored and are stripped by
 *          comstxetxEvaluate, which is where they are checked.
 * @note    The byte after DLE is always data, whatever it is. That is the
 *          whole escape rule, and it is why a payload may contain any byte.
 * @note    An unescaped STX inside an open frame restarts the payload rather
 *          than being stored. A payload STX always arrives escaped, so an
 *          unescaped one can only mean the sender began again.
 * @note    If rxBuffer fills before ETX arrives, the partial frame is
 *          discarded and the driver goes back to looking for STX. Bytes are
 *          ignored while a completed frame is still waiting for
 *          comstxetxEvaluate.
 */
void comstxetxReceive ( comstxetx_t* driver, uint8_t data )
{
    uint8_t store = FALSE;

    if ( driver->rxReadyToEvaluate == FALSE )
    {
        if ( driver->rxFrameOpen == FALSE )
        {
            if ( data == driver->stx )
            {
                driver->rxFrameOpen = TRUE;
                driver->rxIndex = 0;
                driver->rxEscape = FALSE;
            }
            else
            {
                /* Intentionally blank */
            }
        }
        else if ( driver->rxEscape == TRUE )
        {
            driver->rxEscape = FALSE;
            store = TRUE;
        }
        else if ( data == driver->dle )
        {
            driver->rxEscape = TRUE;
        }
        else if ( data == driver->etx )
        {
            driver->rxReadyToEvaluate = TRUE;
        }
        else if ( data == driver->stx )
        {
            driver->rxIndex = 0;
        }
        else
        {
            store = TRUE;
        }

        if ( store == TRUE )
        {
            driver->rxBuffer[ driver->rxIndex ] = data;
            ++driver->rxIndex;

            if ( driver->rxIndex >= driver->rxSize )
            {
                // Terminate all received bytes.
                driver->rxFrameOpen = FALSE;
                driver->rxEscape = FALSE;
                driver->rxIndex = 0;
                driver->rxTimeoutCounter = 0;
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
    else
    {
        /* Intentionally blank */
    }
}
```

The escape branch is tested before the DLE and ETX comparisons, which is what lets a payload byte equal to any framing byte be stored as data.

- [ ] **Step 4: Replace Evaluate**

```c
/**
 * @brief   Called from the main loop; verifies the frame check and runs the
 *          packet callback when a complete frame is waiting.
 * @param[in,out] driver  Framework state.
 * @note    The check runs here rather than in comstxetxReceive because it
 *          walks the whole payload, and comstxetxReceive runs per byte from
 *          an interrupt.
 * @note    The last two stored bytes are the received check, low byte first.
 *          packetProcess is handed everything before them.
 * @note    A frame that fails, and a frame too short to carry a check at all,
 *          are both discarded and counted in rxRejectCount. A frame dropped
 *          with no trace is the worst thing a link can do to whoever has to
 *          diagnose it.
 */
void comstxetxEvaluate ( comstxetx_t* driver )
{
    uint16_t received = 0;
    uint16_t computed = 0;
    uint32_t payloadLength = 0;

    if ( driver->rxReadyToEvaluate == TRUE )
    {
        if ( driver->rxIndex >= COMSTXETX_CHECKSUM_SIZE )
        {
            payloadLength = driver->rxIndex - COMSTXETX_CHECKSUM_SIZE;

            received = ( uint16_t ) driver->rxBuffer[ payloadLength ];
            received = ( uint16_t ) ( received |
                       ( uint16_t ) ( ( uint16_t ) driver->rxBuffer[ payloadLength + 1u ] << 8 ) );

            computed = driver->checksum ( driver->rxBuffer, payloadLength );

            if ( received == computed )
            {
                driver->packetProcess ( driver->rxBuffer, payloadLength );
            }
            else
            {
                ++driver->rxRejectCount;
            }
        }
        else
        {
            ++driver->rxRejectCount;
        }

        driver->rxIndex = 0;
        driver->rxFrameOpen = FALSE;
        driver->rxEscape = FALSE;
        driver->rxReadyToEvaluate = FALSE;
        driver->rxTimeoutCounter = 0;
    }
    else
    {
        /* Intentionally blank */
    }
}
```

- [ ] **Step 5: Key the timeout off rxFrameOpen**

In `comstxetxTimeoutCounter`, change the outer condition from `( driver->rxIndex != 0 )` to `( driver->rxFrameOpen == TRUE )`, and add the two new fields to the discard block:

```c
void comstxetxTimeoutCounter ( comstxetx_t* driver )
{
    if ( ( driver->rxFrameOpen == TRUE ) && ( driver->rxReadyToEvaluate == FALSE ) )
    {
        if ( driver->rxTimeoutCounter > driver->rxTimeout )
        {
            // Terminate all received bytes.
            driver->rxFrameOpen = FALSE;
            driver->rxEscape = FALSE;
            driver->rxIndex = 0;
            driver->rxTimeoutCounter = 0;
        }
        else
        {
            ++driver->rxTimeoutCounter;
        }
    }
    else
    {
        /* Intentionally blank */
    }
}
```

Add a `@note` to its documentation block saying the counter now advances for any open frame, including one that has received no payload byte yet, and that keying it off `rxIndex` would leave such a frame pending for good.

- [ ] **Step 6: Run the tests to verify they pass**

Run the build-and-run command from Step 2. Expected: exit status 0, final line `all checks passed`, no warnings. The five `comat` cases must still pass untouched.

Run the cross-compiler warning check too:

```bash
arm-none-eabi-gcc -c -Wall -Wextra -Iinc/communication src/communication/comstxetx.c -o /dev/null
```

Expected: no output.

- [ ] **Step 7: Commit**

```bash
git add src/communication/comstxetx.c test/Protocol_Test/Protocol_Test.c
git commit -m "+ DLE escaping and frame verification on the comstxetx receive path"
```

---

### Task 3: The frame builder

The transmit side, and the round trip that proves the two halves agree.

**Files:**
- Modify: `src/communication/comstxetx.c`
- Test: `test/Protocol_Test/Protocol_Test.c`

**Interfaces:**
- Consumes: the struct, `comstxetxInit`, and the receive path from Tasks 1 and 2; `COMSTXETX_CHECKSUM_SIZE`.
- Produces: `uint8_t comstxetxBuildFrame ( comstxetx_t* driver, const uint8_t* const payload, uint32_t length, uint32_t* frameLength )`.

- [ ] **Step 1: Write the failing tests**

Add these two case functions above `main`:

```c
/*
 * The decisive case. Build a frame whose payload holds a byte equal to each
 * framing byte, then feed the wire bytes back one at a time and confirm the
 * payload survives unchanged. An encoder that disagrees with the decoder is
 * the defect this design is most exposed to, and nothing else catches it.
 */
static void sxRoundTripCase ( void )
{
    comstxetx_t driver;
    uint8_t rxBuffer[ 64 ];
    uint8_t txBuffer[ 64 ];
    uint8_t wire[ 64 ];
    uint32_t wireLength = 0;
    uint32_t i = 0;
    uint8_t same = TRUE;
    static const uint8_t payload[ ] =
    {
        0x02u, 0x03u, 0x10u, 0x00u, 0xFFu, 0x41u
    };

    printf ( "comstxetx round trip\n" );

    sxReset ( );

    check ( "init", comstxetxInit ( &driver, rxBuffer, txBuffer, 64u, 64u,
                                    0x02u, 0x03u, 0x10u, 10u,
                                    sxSumChecksum, sxPacketProcess ) );

    check ( "the frame was built",
            comstxetxBuildFrame ( &driver, payload,
                                  ( uint32_t ) sizeof ( payload ), &wireLength ) );

    /*
     * Three payload bytes need an escape and none of the checksum bytes do:
     * the sum of the payload is 0x0155, so the check bytes are 0x55 and 0x01.
     * STX + 6 payload bytes + 3 escapes + 2 check bytes + ETX is 13.
     */
    check ( "the wire length accounts for the escapes",
            ( uint8_t ) ( wireLength == 13u ) );
    check ( "the frame opens with STX", ( uint8_t ) ( txBuffer[ 0 ] == 0x02u ) );
    check ( "the frame closes with ETX",
            ( uint8_t ) ( txBuffer[ wireLength - 1u ] == 0x03u ) );

    for ( i = 0; i < wireLength; ++i )
    {
        wire[ i ] = txBuffer[ i ];
    }

    sxFeedBytes ( &driver, wire, wireLength );
    comstxetxEvaluate ( &driver );

    check ( "the frame reached packetProcess", ( uint8_t ) ( sxProcessCalls == 1u ) );
    check ( "the payload length survived",
            ( uint8_t ) ( sxLastLength == ( uint32_t ) sizeof ( payload ) ) );

    for ( i = 0; i < ( uint32_t ) sizeof ( payload ); ++i )
    {
        if ( sxLastFrame[ i ] != payload[ i ] )
        {
            same = FALSE;
        }
    }

    check ( "and every payload byte survived", same );
    check ( "nothing was rejected",
            ( uint8_t ) ( comstxetxGetRejectCount ( &driver ) == 0u ) );
}

/*
 * BuildFrame validates its own arguments because they are new and can break a
 * later invariant, and it refuses rather than writing past the buffer it was
 * given. crc16 is used here to prove the callback type takes the library's own
 * CRC with no wrapper.
 */
static void sxBuildGuardCase ( void )
{
    comstxetx_t driver;
    uint8_t rxBuffer[ 64 ];
    uint8_t txBuffer[ 8 ];
    uint32_t wireLength = 99u;
    static const uint8_t payload[ ] =
    {
        0x02u, 0x02u, 0x02u, 0x02u, 0x02u, 0x02u
    };

    printf ( "comstxetx build guards\n" );

    check ( "init takes crc16 directly",
            comstxetxInit ( &driver, rxBuffer, txBuffer, 64u, 8u,
                            0x02u, 0x03u, 0x10u, 10u,
                            crc16, sxPacketProcess ) );

    check ( "a NULL payload is rejected",
            ( uint8_t ) ( comstxetxBuildFrame ( &driver, NULL, 1u, &wireLength ) == FALSE ) );
    check ( "a NULL length pointer is rejected",
            ( uint8_t ) ( comstxetxBuildFrame ( &driver, payload, 1u, NULL ) == FALSE ) );

    /*
     * Every payload byte equals STX, so each one costs two wire bytes. The
     * frame would need 1 + 12 + at least 2 + 1 bytes and the buffer holds 8.
     */
    check ( "a frame that would overflow txBuffer is refused",
            ( uint8_t ) ( comstxetxBuildFrame ( &driver, payload,
                                                ( uint32_t ) sizeof ( payload ),
                                                &wireLength ) == FALSE ) );
    check ( "and the reported length is zeroed",
            ( uint8_t ) ( wireLength == 0u ) );
}
```

Add their calls in `main` after `sxOpenEmptyTimeoutCase ( );`:

```c
    printf ( "\n" );
    sxRoundTripCase ( );
    printf ( "\n" );
    sxBuildGuardCase ( );
```

- [ ] **Step 2: Run the tests to verify they fail**

Run the build-and-run command. Expected: FAIL to link, `undefined reference to 'comstxetxBuildFrame'`.

- [ ] **Step 3: Implement BuildFrame**

Append to `src/communication/comstxetx.c`, before the accessor:

```c
/**
 * @brief   Builds a complete wire frame in txBuffer from a payload.
 * @param[in,out] driver       Framework state. txBuffer is written.
 * @param[in]     payload      Bytes to carry. Any byte value is allowed.
 * @param[in]     length       Number of payload bytes. Zero is legal and
 *                             produces a frame carrying only the check.
 * @param[out]    frameLength  Number of bytes written to txBuffer.
 * @return  TRUE on success, FALSE when an argument is NULL or the escaped
 *          frame would not fit in txBuffer.
 * @note    On FALSE the contents of txBuffer are undefined and frameLength is
 *          set to zero. The function does not pre-check the worst case of
 *          1 + 2 * ( length + 2 ) + 1, because that would refuse frames that
 *          fit comfortably whenever few payload bytes need an escape.
 * @note    It does not transmit. comstxetx has no transmission trigger the
 *          way comat does; the buffer is filled and the length reported, and
 *          sending is the caller's.
 * @note    The check is computed over the unescaped payload, so escaping
 *          cannot change it. The check bytes are themselves escaped, without
 *          which a check byte equal to etx would close the frame it protects.
 */
uint8_t comstxetxBuildFrame ( comstxetx_t* driver, const uint8_t* const payload,
                              uint32_t length, uint32_t* frameLength )
{
    uint8_t retVal = FALSE;
    uint8_t overflow = FALSE;
    uint8_t byte = 0;
    uint8_t needed = 0;
    uint16_t sum = 0;
    uint32_t i = 0;
    uint32_t out = 0;
    uint8_t check[ COMSTXETX_CHECKSUM_SIZE ];

    if ( ( driver != NULL ) && ( payload != NULL ) && ( frameLength != NULL ) )
    {
        sum = driver->checksum ( payload, length );
        check[ 0 ] = ( uint8_t ) ( sum & 0xFFu );
        check[ 1 ] = ( uint8_t ) ( ( sum >> 8 ) & 0xFFu );

        // txSize is at least four, so the STX always fits.
        driver->txBuffer[ out ] = driver->stx;
        ++out;

        for ( i = 0; ( i < ( length + COMSTXETX_CHECKSUM_SIZE ) ) && ( overflow == FALSE ); ++i )
        {
            if ( i < length )
            {
                byte = payload[ i ];
            }
            else
            {
                byte = check[ i - length ];
            }

            if ( ( byte == driver->stx ) || ( byte == driver->etx ) ||
                    ( byte == driver->dle ) )
            {
                needed = 2;
            }
            else
            {
                needed = 1;
            }

            // The trailing ETX is reserved here so the frame cannot fail late.
            if ( ( out + needed + 1u ) > driver->txSize )
            {
                overflow = TRUE;
            }
            else
            {
                if ( needed == 2 )
                {
                    driver->txBuffer[ out ] = driver->dle;
                    ++out;
                }
                else
                {
                    /* Intentionally blank */
                }

                driver->txBuffer[ out ] = byte;
                ++out;
            }
        }

        if ( overflow == FALSE )
        {
            driver->txBuffer[ out ] = driver->etx;
            ++out;

            *frameLength = out;
            retVal = TRUE;
        }
        else
        {
            *frameLength = 0;
            retVal = FALSE;
        }
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}
```

The loop walks `length + 2` items and reads the last two from `check`, so the payload and the check bytes go through one escape path rather than two copies of it.

- [ ] **Step 4: Run the tests to verify they pass**

Run the build-and-run command. Expected: exit status 0, `all checks passed`, no warnings. Then the cross-compiler check:

```bash
arm-none-eabi-gcc -c -Wall -Wextra -Iinc/communication src/communication/comstxetx.c -o /dev/null
```

Expected: no output.

- [ ] **Step 5: Commit**

```bash
git add src/communication/comstxetx.c test/Protocol_Test/Protocol_Test.c
git commit -m "+ Frame builder for comstxetx"
```

---

### Task 4: Banners, guide, and tree-wide verification

**Files:**
- Modify: `src/communication/comstxetx.c` (banner only)
- Modify: `src/communication/comat.c` (banner only)
- Modify: `CLAUDE.md`

**Interfaces:**
- Consumes: the finished module from Tasks 1-3.
- Produces: nothing the code depends on.

- [ ] **Step 1: Update the comstxetx banner**

Bump `@version` from `0.0.4` to `1.0.0` — the frame format changed and callers must adapt. Append to `@par History`, matching the existing format exactly, with each line ending `@n`:

```
  * 05/08/2026 Breaking change. Frames now carry a DLE escape and a @n
  *            two byte check. Init takes the escape byte and a @n
  *            checksum callback, and packetProcess receives the @n
  *            payload alone, without the STX byte and without the @n
  *            check bytes. Existing callers will not compile, which @n
  *            is deliberate: a format change that still compiled @n
  *            would corrupt a working link silently. @n
```

Add a `@note` to the file banner recording what the module can now carry:

```
  * @note      Any payload byte value is allowed. A byte equal to stx, etx or
  *            dle travels preceded by dle, and the byte after dle is always
  *            data.
```

- [ ] **Step 2: Add the comat banner sentence**

`comat.c` gets no code change. Add a `@note` to its file banner so the next reader does not "fix" the missing integrity check:

```
  * @note      This module carries no checksum on purpose. AT is an ASCII
  *            command protocol and its real peers, from modems to cellular
  *            and BLE modules, do not checksum their frames. Adding one here
  *            would invent a private dialect no peer speaks. comstxetx is
  *            the module for links that need integrity.
```

Bump `comat.c`'s `@version` from `0.0.4` to `0.0.5` and append a `05/08/2026` history line naming the note as the only change.

- [ ] **Step 3: Update CLAUDE.md**

Three edits, each verified rather than assumed.

1. The Testing section says 190 exported symbols. This work adds two. Verify by rebuilding every object and counting, the way CLAUDE.md's own Verification section describes, and write the number you measure:

```bash
mkdir -p /tmp/objs
for f in src/*/*.c drv/*.c; do m=$(basename $(dirname "$f")); inc="inc/$m"; [ -d "$inc" ] || inc="drv"; \
  arm-none-eabi-gcc -c -Wall -I"$inc" -Idrv "$f" -o "/tmp/objs/$(basename ${f%.c}).o"; done
arm-none-eabi-nm /tmp/objs/*.o | grep ' T ' | awk '{print $3}' | sort -u | wc -l
```

2. Add this paragraph after the `softtimer` one in the opening section. Check every claim in it against the code as it stands before committing it, and correct anything that does not match:

```markdown
The two protocol modules answer different problems and only one of them checks what it receives. `comstxetx` frames binary: a payload byte equal to STX, ETX or the caller-chosen DLE travels preceded by DLE, and the byte after DLE is always data, so any byte value can cross the link. Every frame carries a two-byte check computed over the unescaped payload by a function the caller installs at `Init` — the signature is `crc16`'s, so `crc16` goes in directly, and the indirection is what lets the module keep its independence from `crc/`. `comstxetxBuildFrame` is the matching encoder, filling the transmit buffer the module owns, so the two halves cannot drift apart. A frame that fails its check is dropped and counted in `comstxetxGetRejectCount` rather than silently. `comat` has none of this on purpose: it speaks AT, an ASCII command protocol whose real peers do not checksum, and adding one would invent a dialect nothing else speaks.
```

3. The table of tests that pin a specific bug already has a `Protocol_Test` row. Extend that row rather than adding a second one for the same test — replace it with:

```markdown
| `Protocol_Test` | `rxTimeoutCounter` running on across frames. Note that the tick-driven discard cleared the counter even before the fix, so only a sequence that **completes** a frame late in its budget and then asks the next one for a full budget discriminates. Same for the buffer-overflow reset path. Also a `comstxetx` frame opened on STX but carrying no payload byte never timing out, which is what keying the timeout off `rxIndex` rather than `rxFrameOpen` causes. |
```

- [ ] **Step 4: Verify the whole tree**

```bash
for f in src/*/*.c drv/*.c; do m=$(basename $(dirname "$f")); inc="inc/$m"; [ -d "$inc" ] || inc="drv"; \
  arm-none-eabi-gcc -c -Wall -Wextra -I"$inc" -Idrv "$f" -o /dev/null; done
```

Expected: no output at all.

```bash
for h in inc/*/*.h drv/*.h; do echo "#include \"$(basename $h)\""; done > /tmp/allhdr.c
echo "int main(void){return 0;}" >> /tmp/allhdr.c
arm-none-eabi-gcc -c -Wall $(for d in inc/*/ drv/; do echo -n " -I$d"; done) /tmp/allhdr.c -o /dev/null
```

Expected: no output.

Confirm both new symbols are prefixed and referenced by the test:

```bash
arm-none-eabi-nm /tmp/objs/comstxetx.o | grep ' T ' | awk '{print $3}' | sort -u
```

Expected, exactly six lines, every one starting `comstxetx`:

```
comstxetxBuildFrame
comstxetxEvaluate
comstxetxGetRejectCount
comstxetxInit
comstxetxReceive
comstxetxTimeoutCounter
```

- [ ] **Step 5: Run every test, not only the protocol one**

This work changed a module other tests do not touch, but the tree-wide run is what proves nothing else regressed:

```bash
export PATH="$LOCALAPPDATA/Microsoft/WinGet/Packages/BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe/mingw64/bin:$PATH"
gcc -Wall -Wextra -Iinc/communication -Iinc/crc \
  test/Protocol_Test/Protocol_Test.c src/communication/comat.c src/communication/comstxetx.c src/crc/crc16.c \
  -o /tmp/protocol_test && /tmp/protocol_test
```

Expected: `all checks passed` and exit status 0.

- [ ] **Step 6: Commit**

```bash
git add src/communication/comstxetx.c src/communication/comat.c CLAUDE.md
git commit -m "+ Record the comstxetx frame format change"
```

- [ ] **Step 7: Report what was verified**

Unlike earlier work in this repository, these tests were executed. The report should say so plainly: the protocol test builds and runs, the exact pass line it printed, the measured symbol count, and that the tree-wide compile and the all-headers translation unit are clean. If any expected value in the new test cases had to be corrected during implementation, say which and why — a hand-computed checksum that turned out wrong is worth recording, not quietly fixing.
