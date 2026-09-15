# Library Rules

This file holds the rules that shape the *library*: what a module may depend on,
how it is laid out, and what a new one has to look like. The rules that shape the
*code inside* a module — brace style, naming, the `Init` contract, `const`, the
Doxygen tag set — live in `codingReference.md` and are not repeated here.

## 1. Freestanding

The library targets bare metal.

- No heap. `malloc`, `calloc`, `realloc` and `free` appear nowhere, ever.
- No operating system, no threads, no file I/O, no `errno`.
- No `printf` or any other stdio call in `src/` or `drv/`. Printing belongs in
  `test/` and `sample/` only.
- Fixed-width types from `<stdint.h>` throughout. A bare `int`, `long` or
  `unsigned` in an interface is a portability bug.
- The only standard headers a module may include are `<stdint.h>`, `<stddef.h>`
  and `<math.h>`, and the last one only where it is genuinely used.
- C89-compatible style. Declarations at the top of a block, no variable-length
  arrays, no designated initializers.

## 2. Module independence

**No module includes another module's header.** Every `.c` includes its own
header and nothing else from this tree.

This is the rule the whole layout exists to protect. The library is consumed by
copying a module's `.h`/`.c` pair into a target project, so a cross-module
include turns a one-file copy into a dependency hunt. Where two modules would
otherwise share code, the code is duplicated and the file banner says so — see
the bracketing search in `interp.c`, copied from `searchUpperBound`.

Where a module genuinely needs something another one produces, it takes it as a
parameter or as a callback installed at `Init`. `comstxetx` verifies frames with
a checksum function whose signature is the one `crc16` already has, so `crc16`
goes in directly and neither module knows about the other. `encoder` consumes
what two `bininp` outputs produce and includes neither header.

## 3. Layout

```
inc/<group>/<name>.h   <-->  src/<group>/<name>.c   strict 1:1 pair
drv/<name>.h, <name>.c                              hardware drivers, side by side
template/inc/generic.h, template/src/generic.c      copy these to start a module
test/<Name>_Test/                                   one standalone main() per module
sample/                                             standalone examples, not library code
```

A header with no source, or a source with no header, is a defect. There are no
exceptions: the four stubs this rule used to carve out — `comgenbuf`,
`matrixlib`, `comsafe` and `comsec` — were implemented on 15/09/2026.

There is no build system and there is not meant to be one. Nothing in this tree
produces a shippable artifact. `run_tests.sh` builds and runs the tests and
ships nothing.

## 4. Header contract

Every header is `template/inc/generic.h` with content filled into fixed
sections. All of the sections stay, including the empty ones:

```c
#ifndef <NAME>_H_
#define <NAME>_H_
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
/* ENUMS */
/* EXTERNS */
/* FUNCTION PROTOTYPES */
```

`TRUE` and `FALSE` are redefined per header, guarded by `#ifndef`, precisely so
that a module stays self-contained when it is copied out alone.

Enums use a `SCREAMING_CASE` tag with short prefixed members: `BS_EMPTY`,
`BB_OVERWRITE`, `HC595_DLY_MS`.

Every header must be includable on its own, and all of them together must
compile in one translation unit. That second check is what catches a duplicate
include guard or a clashing typedef.

## 5. The driver-struct pattern

Every stateful module has the same shape, and a new one must match it.

- One `typedef struct { ... } <prefix>_t;` holding all state. Always a typedef —
  the caller never writes the `struct` keyword.
- **The caller owns all storage.** Buffers are passed into `Init` as pointers.
  The module never allocates and never holds static state of its own. That is
  checkable rather than aspirational: a writable static lands in `.data` or
  `.bss`, so both must be empty in every module object, and `scripts/check.sh`
  fails the tree when one is not. A read-only table is `.rodata` and is fine —
  it is `crc16`'s lookup table, not state.
- The first parameter of every function is `<prefix>_t* driver`.
- Names are the module prefix plus a verb: `xxxInit`, then
  `xxxUpdate` / `xxxIteration` / `xxxControl` / `xxxReceive`, then
  `xxxGetValue` / `xxxGetOutput`.
- Hardware and I/O are injected as function pointers stored in the struct at
  `Init`. **Library code never calls a HAL directly.** See `drv/hc595_drv.h`
  (`sckDrv`, `rckDrv`, `datDrv`, `dlyMs`, `dlyNop`) and
  `inc/communication/comat.h` (`packetProcess`, `txTransmissionTrigger`).
- Protocol modules are byte-driven state machines: `xxxReceive` per byte from
  the ISR, `xxxEvaluate` from the main loop, `xxxTimeoutCounter` from a periodic
  tick.

A module that owns no caller storage and no callbacks is a value type rather
than a driver, and is exempt — `complex_t` is the only one.

## 6. Time

The library has no clock. A module that needs time takes it one of two ways:

- **A sample period at `Init`**, as `pid` and `ramp` do, so limits and gains are
  written in units per second rather than per call.
- **A tick the caller drives from a fixed-rate ISR**, as `softtimer`,
  `comatTimeoutCounter` and `hc595Interrupt` do. The interrupt rate *is* the
  unit, and periods are expressed in ticks.

`softtimer` is the only time abstraction, and by rule 2 no other module may use
it. Modules that need a counter keep their own.

## 7. Width variants

A module that is useful without an FPU carries integer variants beside the float
one, in the same file, named with a type suffix: `mafIterationi32`,
`circBufAddu32`, `statVariancei32`. These are width variants of one module, not
modules of their own.

- An integer variant carries every intermediate wide enough that the documented
  input range cannot overflow it. `interpCalculatei32` and `mathMapi32` use
  `int64_t` for exactly this reason.
- An integer **division** rounds to nearest rather than truncating, and accounts
  for the sign of the divisor where the divisor may be negative. `mathMapi32`,
  `mathLerpi32` and `interpCalculatei32` all add half the magnitude of the
  divisor before dividing, which is the form to copy.
- A Q16 **shift** may truncate where the half LSB it loses does not accumulate,
  and `emafIterationi32` and `alphabetaIterationi32` do. It rounds where the
  loss does accumulate: `biquadIterationi32` shifts inside a feedback path and
  puts a measured half count of standing offset on a symmetric signal without
  it. Either way the file says which it does and why.
- A fixed-point variant states its Q format in the file banner and in the
  `@brief` of every function that takes or returns a scaled value.
- **And it states what it costs instead.** A variant written to avoid the
  software float routines pulls in 64-bit integer helpers in their place —
  `__aeabi_lmul` for a Q16 multiply, `__aeabi_ldivmod` for a Q16 divide, neither
  of which a Cortex-M0 has an instruction for. That is the right trade, because
  an `int64_t` intermediate is what keeps the variant correct, but the claim
  "for parts with no FPU" is incomplete without the other half.
  `sh scripts/runtime.sh` measures it.
- A width exists because a caller needs it, not for symmetry. `mathMap` has no
  `u32` variant because a `u32` map cannot express the descending input range
  that would make it worth having.

## 8. Preconditions and error handling

- A precondition that can be checked once is checked at `Init`, not per call.
  `interpInit` verifies that the table ascends strictly so that
  `interpCalculate` can divide without testing the divisor.
- A precondition the module cannot check is documented and the caller carries
  it. The binary searches require an ascending array and give a confident wrong
  answer without one; `sortIsSorted` exists so the caller can check it cheaply.
- **A module never guesses.** When the input is ambiguous the module refuses and
  counts the event, so the caller can see it: `encoderGetErrorCount` counts a
  missed quadrature step rather than inventing a direction, and
  `comstxetxGetRejectCount` counts a frame that failed its check rather than
  swallowing it.
- A status return is never optional. `hc595OneShot` and `hc595Start` both report
  whether they took the pins; ignoring the answer silently skips a transfer.

## 9. Testing

- Every module with functions has a test under `test/<Name>_Test/`, a standalone
  `main()` that builds from the test file plus the module sources its
  `#include "..."` lines name.
- **And the stacks have one too.** Modules may not include each other, so they
  only ever meet in caller code, and whether their units, buffer sizes and
  callback shapes line up is not a property any single-module test can check.
  `test/Integration_Test/` is that caller code: it asserts the three real
  stacks — the receive path, the motion loop and the measurement chain — and it
  is where a defect that exists only at a seam belongs.
- **Every exported symbol is referenced by at least one test.**
- New tests assert and return non-zero on failure. They do not print values for
  a human to compare — seven older tests do, and they are legacy, not a pattern
  to follow.
- Expected values come from an independent model — a hand calculation, a
  published vector, a transliteration of the algorithm — never from running this
  implementation and recording what it said.
- When a bug is fixed, the test gets a check aimed at that specific bug, so the
  regression fails rather than passing quietly. `CLAUDE.md` keeps the table of
  which test pins which bug.
- **A test that claims `Init` writes a field has to poison the struct first.**
  A driver is a stack local, so the memory is usually zero already or holds the
  previous case's values, and a missing assignment reads as if it had happened.
  `Control_Test` fills the driver with a non-zero byte pattern before each
  `pidInit` for exactly this reason.
- **A check on one width proves nothing about the others.** Each width carries
  its own copy of the code, so a degenerate length or a bound that wraps has to
  be checked on every width that has one, not on whichever was convenient.
- **And the pin gets a mutation.** A file under `scripts/mutations/`
  reintroduces the defect and names the test that has to fail because of it.
  Without one, a pin that stops biting — an assertion softened while tidying a
  test — is exactly the silent failure the pin existed to prevent, and nothing
  would notice. `sh scripts/mutate.sh` applies each in turn and reports any
  that survived.

## 10. Verification before commit

Every `.c` under `src/` and `drv/` compiles clean under `-Wall -Wextra`. **Zero
warnings, no exceptions** — a new warning is a regression, not background noise.

```bash
sh run_tests.sh              # build and run every test
sh run_tests.sh Ramp_Test    # or just one, mid-change
sh scripts/samples.sh        # build and run every example under sample/
sh scripts/check.sh          # warnings, headers, symbol coverage, static storage
STRICT=1 sh scripts/check.sh # the same, under -Wconversion and its neighbours
sh scripts/mutate.sh         # every known defect still fails its test
sh scripts/size.sh           # code size per module
sh scripts/runtime.sh        # which compiler runtime helpers each module needs
```

The first five are gates and CI runs all of them on every push. **The strict
profile is a gate, not an aspiration**: the tree was clean under
`-Wconversion -Wsign-conversion -Wshadow -Wdouble-promotion -Wcast-qual` on
15/09/2026, the eight warnings it had found were all one pattern — an array
length converted to float for a divide — and they are written out now.

`size.sh` is a report. `runtime.sh` is mostly one too, with a single rule in
it: nothing here may pull in a double precision helper, because this library is
single precision throughout and a `__aeabi_d*` means a `double` slipped into an
expression.

Commit messages are terse and prefixed: `+` for an addition, `*` for a fix or an
update. `+ bininpGetRisingValue function`, `* bugfix`.
