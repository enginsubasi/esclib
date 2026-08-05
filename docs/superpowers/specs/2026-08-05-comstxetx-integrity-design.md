# Transparency and Integrity for `comstxetx`

Date: 2026-08-05
Status: approved

## Problem

`comstxetx` frames bytes between a caller-chosen STX and ETX. It cannot carry arbitrary binary data, and it does not check what it receives.

The transparency hole is the more serious of the two. `comstxetxReceive` closes the frame on any byte equal to ETX:

```c
if ( data == driver->etx )
{
    driver->rxReadyToEvaluate = TRUE;
}
```

There is no escape mechanism, so a payload byte that happens to equal ETX truncates the frame. A payload byte equal to STX is stored as ordinary data with no resynchronisation. The module therefore only works for payloads drawn from an alphabet that avoids the delimiters — a constraint nothing in the code or the documentation states.

The integrity hole sits on top of it. Neither `comat` nor `comstxetx` contains a single CRC or checksum call. A frame corrupted in transit that still has valid framing is handed to `packetProcess` as though it were sound.

A checksum alone would not fix the first problem and would be damaged by it: a checksum byte equal to ETX would truncate the very frame it protects. Transparency is a prerequisite, so both are solved together.

There is a third finding that shapes the work. `comstxetx` takes `txBuffer` and `txSize` at `Init`, validates them, zero-fills them — and never touches them again. Unlike `comat` it has no transmit path at all. That dead parameter becomes the natural home for the frame builder, and building an escaped, checksummed frame by hand in every caller is exactly where an encoder/decoder mismatch would hide.

## Decisions

Five questions were settled before design.

1. **`comat` is out of scope and stays as it is.** It implements AT commands — an ASCII protocol whose real peers, from modems to cellular and BLE modules, carry no checksum. Adding one would invent a private dialect no peer speaks. The absence of integrity checking there is the protocol's nature, not a defect. `comat.c` gains one sentence in its banner saying so, to stop the next reader from "fixing" it.

2. **Escaping is DLE followed by the byte itself, not the PPP-style `byte XOR 0x20`.** The XOR form is more robust in the general case — an escaped byte never resembles a delimiter on the wire — but this module lets the *caller* choose the delimiters. A caller picking `stx = 0x22` and `etx = 0x02` would find that escaping `0x02` produces `0x22`, which is STX. Making the XOR form safe would need three further collision checks at `Init` and would leave a subtle failure mode. The literal form has no such interaction: the rule is one sentence, and the byte after DLE is always data.

3. **The checksum is mandatory.** `Init` is a breaking change regardless, because it gains the DLE byte. An optional checksum would mean two frame formats on the wire, a doubled test matrix, and a NULL branch on every frame. One format is simpler and there is no silent no-integrity mode.

4. **Verification happens in `comstxetxEvaluate`, not in `comstxetxReceive`.** A checksum over the payload is O(n) and `Receive` runs per byte from an interrupt. This follows the split the library already draws: `Receive` is byte work, `Evaluate` is main-loop work.

5. **`packetProcess` receives the payload alone** — no STX, no checksum bytes. The module currently stores STX at index 0 but not ETX, which is inconsistent both internally and with `comat`, where every byte of the frame including the leading `AT` and trailing CR LF is stored. Since the frame format is changing anyway, payload-only is the clean endpoint.

## Wire format

```
STX | escaped( payload ) | escaped( checksum low ) | escaped( checksum high ) | ETX
```

The checksum is computed over the **unescaped payload only**. STX, ETX and the checksum bytes themselves are not covered. Escaping is a wire-level transformation applied after the value is computed, so what the sender computes and what the receiver recomputes are the same bytes regardless of how many escapes the wire needed.

The checksum bytes are themselves subject to escaping. Without that, a `crc16` result with a byte equal to ETX would close the frame early.

Low byte first. Encoder and decoder live in the same file and must agree; the order is documented so a third-party implementation of the other end can match it.

## Placement

No new module. `inc/communication/comstxetx.h` and `src/communication/comstxetx.c` change in place. `comat` is untouched apart from the banner sentence in decision 1.

## State

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

The `volatile` split follows the ruling already made for `softtimer_t` and the precedent in `hc595_t`: the live receive state is shared between `comstxetxReceive` on the interrupt side and `comstxetxEvaluate` on the caller side, and is declared volatile so the writes stay ordered and neither side caches state in a register. Configuration written once at `Init` is not. `rxRejectCount` is written and read from the main loop only, so it stays plain.

`rxFrameOpen` is new and is not decoration. Today `rxIndex == 0` carries two meanings, "no frame in progress" and "no bytes yet". Once STX is no longer stored, a frame that is open but has received no payload byte is a legitimate state, and the two meanings have to separate. `comstxetxTimeoutCounter` must key off `rxFrameOpen`; keying off `rxIndex` would leave an opened-but-empty frame pending forever.

## API

```c
uint8_t comstxetxInit ( comstxetx_t* driver,
                        uint8_t* rxBuffer, uint8_t* txBuffer,
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

Two new exported symbols, from four to six.

The checksum callback's signature is exactly that of `crc16 ( const uint8_t* const array, uint32_t size )`, so a caller passes `crc16` directly with no wrapper. `crc16Alt` fits the same shape.

`comstxetxBuildFrame` returns a status because it takes new arguments that can break a later invariant, the same reason `pidChangeCoefficients` returns one.

## Receive

Per byte, from the interrupt. Bytes are ignored while a completed frame waits for `comstxetxEvaluate`, as today.

- **Not in a frame:** a byte equal to STX opens one — `rxFrameOpen = TRUE`, `rxIndex = 0`, `rxEscape = FALSE`. Anything else is ignored.
- **In a frame, escape pending:** store the byte, clear `rxEscape`, apply the overflow check. This branch comes first, so a payload byte equal to STX, ETX or DLE is stored as data.
- **In a frame, byte equals DLE:** set `rxEscape`. Nothing is stored.
- **In a frame, byte equals ETX:** close the frame, `rxReadyToEvaluate = TRUE`. Verification is not done here.
- **In a frame, byte equals STX:** resynchronise — `rxIndex = 0`, `rxEscape = FALSE`, the frame stays open. A payload STX always arrives escaped, so an unescaped one can only mean the sender restarted. This is what delimiter framing buys over a length prefix.
- **Otherwise:** store the byte and apply the overflow check.

The overflow check keeps the existing shape: store, then compare the index against `rxSize`, and on overflow discard the frame — `rxFrameOpen = FALSE`, `rxIndex = 0`, `rxTimeoutCounter = 0`.

## Evaluate

When a frame is waiting:

- Fewer than two stored bytes — the frame cannot carry a checksum. Discard it and increment `rxRejectCount`.
- Otherwise the last two stored bytes are the received checksum, low byte first. Recompute over the preceding `rxIndex - 2` bytes and compare. On a match call `packetProcess ( rxBuffer, rxIndex - 2 )`. On a mismatch discard and increment `rxRejectCount`.

Either way the driver resets: `rxFrameOpen = FALSE`, `rxIndex = 0`, `rxReadyToEvaluate = FALSE`, `rxEscape = FALSE`, `rxTimeoutCounter = 0`.

A silently dropped frame is the worst thing a serial link can do to whoever has to diagnose it in the field. `rxRejectCount` costs four bytes and is read with `comstxetxGetRejectCount`. It counts both rejection causes; distinguishing them would be two counters for no decision anyone makes differently.

## BuildFrame

Writes STX, the escaped payload, the escaped checksum low byte, the escaped checksum high byte and ETX into `txBuffer`, and reports the wire length through `frameLength`.

Validation: `driver`, `payload` and `frameLength` non-NULL. A zero `length` is legal and produces a frame carrying only the checksum.

On overflow the function returns `FALSE` and writes `0` through `frameLength`; the contents of `txBuffer` are then undefined. It does not pre-check the worst case of `1 + 2 * ( length + 2 ) + 1`, because that would refuse frames that fit comfortably whenever the payload needs few escapes or none.

It does not transmit. `comstxetx` has no `txTransmissionTrigger` the way `comat` does, and adding one is a larger change than this work needs. The buffer is filled and the length reported; sending is the caller's.

## Validation at Init

`comstxetxInit` writes nothing to the driver when any check fails:

- `driver`, `rxBuffer` and `txBuffer` non-NULL, using `NULL` from `<stddef.h>`.
- `stx`, `etx` and `dle` pairwise distinct. Three comparisons, replacing today's single `stx != etx`. Two equal delimiters make at least one of them unreachable as itself.
- `rxSize >= 2`. The smallest meaningful frame carries an empty payload and two checksum bytes, and `comstxetxReceive` stores a byte before it compares the index against `rxSize`. `COMSTXETX_MIN_RX_SIZE` keeps its value and gains a new reason.
- `txSize >= 4`. The smallest frame `comstxetxBuildFrame` can emit is STX, two checksum bytes and ETX, with no escaping.
- `checksum` and `packetProcess` non-NULL. Both are called without checking.

## Breaking change

`Init` gains two parameters and `packetProcess` sees different indices. Existing callers will not compile, which is the right outcome — a change of frame format that compiled and ran would corrupt a working link silently.

The `@par History` entry must say so plainly, and the version bumps to a new major.

## Testing

Assert style, added to `test/Protocol_Test/Protocol_Test.c`.

The decisive case is the round trip: build a frame whose payload deliberately contains bytes equal to STX, ETX and DLE, feed the resulting wire bytes to `comstxetxReceive` one at a time, and confirm `packetProcess` sees the original payload byte for byte. Nothing else proves the encoder and the decoder agree, and an encoder/decoder mismatch is the defect this design is most exposed to.

Alongside it:

- A payload byte corrupted after the frame is built is rejected, `packetProcess` is not called, and `rxRejectCount` reads 1.
- A frame closed with fewer than two stored bytes is rejected and counted.
- An unescaped STX inside an open frame restarts the payload rather than being stored.
- An escape sequence arriving as the buffer fills takes no write past `rxSize`.
- `comstxetxBuildFrame` returns `FALSE` and reports a zero length when the escaped frame would exceed `txSize`.
- `comstxetxInit` rejects `stx == etx`, `dle == stx`, `dle == etx`, a NULL checksum, a NULL `packetProcess`, and `txSize < 4`, leaving the driver untouched each time.
- **The pinned regression:** a frame that has opened on STX but received no payload byte still times out. Keying the timeout off `rxIndex` rather than `rxFrameOpen` leaves it pending forever, and every other case in the file passes with that bug present.

The existing `Protocol_Test` cases for `comat` are untouched, and the case pinning `rxTimeoutCounter` running on across frames must keep passing for both modules.
