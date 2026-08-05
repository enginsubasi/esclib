# interp — table interpolation

Date: 2026-08-05
Status: approved

## The problem

`searchClosest` answers which table entry to read, and CLAUDE.md points at it as what a calibration or linearisation table needs. It returns an index, so the caller gets one of the values that are actually in the table. Nothing in the library returns a value *between* two entries, so every caller that needs one writes the bracketing search and the interpolation arithmetic by hand.

That arithmetic is short, which is exactly why it gets written badly. The two failures it invites are a divisor of zero when two x entries are equal, and a bracket index of `length - 1` that reads one past the end. Both produce a confident wrong answer rather than a crash.

## Shape: a driver, not a stateless function

The backlog entry proposed a stateless value module like `complex`. That is wrong for this module, and the reason is the library's own rule.

Ascending order is a precondition here, the same way it is for `searchBinary`, and CLAUDE.md already records what a violated precondition costs: the binary searches "give a confident wrong answer without it", which is why `sortIsSorted` exists to check it cheaply. A stateless `interp` has nowhere to make that check. Verifying the order on every call is O(N) on a path that runs per sample, so in practice it would not be checked at all.

A driver moves the check to where the library says checks belong. `interpInit` verifies the table once, at boot, and that matches how a calibration table is actually used — set up once, read thousands of times. The Init contract then holds without an exception: validation happens at `Init` and nowhere else, and `interpCalculate` may dereference the driver unchecked because the caller already got a yes or no.

Reverse lookup needs no function of its own. A second `interp_t` initialized with the x and y tables exchanged reads the same curve backwards, at the cost of one more struct and no new code. It is the caller's business whether their y table is monotonic enough for that to mean anything.

## Layout

```
inc/math/interp.h
src/math/interp.c
test/Interp_Test/Interp_Test.c
```

The module joins the `math` group beside `basicmath` and `statistic` as its own 1:1 header and source pair. Its prefix is `interp`, and it goes in CLAUDE.md's list of stateful modules that use their own name.

## Interface

Two structs and two function triples, following the `circBufu32_t` / `circBufu8_t` precedent for a module that is width-specific.

```c
typedef struct
{
    const float* xTable;
    const float* yTable;
    uint32_t length;
} interp_t;

typedef struct
{
    const int32_t* xTable;
    const int32_t* yTable;
    uint32_t length;
} interpi32_t;
```

```c
uint8_t interpInit      ( interp_t* driver, const float* const xTable,
                          const float* const yTable, uint32_t length );
float   interpCalculate ( const interp_t* const driver, float x );
uint8_t interpInRange   ( const interp_t* const driver, float x );

uint8_t interpIniti32      ( interpi32_t* driver, const int32_t* const xTable,
                             const int32_t* const yTable, uint32_t length );
int32_t interpCalculatei32 ( const interpi32_t* const driver, int32_t x );
uint8_t interpInRangei32   ( const interpi32_t* const driver, int32_t x );
```

The tables are stored as pointers to `const`. A calibration table lives in flash, and the module never writes to one, so anything else would force the caller to cast the qualifier away — the same reason `crc16` and `statVariance` take `const T* const`.

Both compute functions take a `const` driver. They are pure reads, which is what lets one driver be shared between the main loop and an interrupt without a second thought. That property is the reason the design does not cache the last bracket index: caching would make `interpCalculate` a write, cost the `const`, and buy nothing on the small tables this library is used with. An eight-entry NTC table costs three comparisons to search.

## interpInit

Returns `TRUE` on success and `FALSE` on a rejected argument, writing nothing to the driver on failure. It rejects:

- a NULL `driver`, `xTable` or `yTable`, tested against `NULL` from `<stddef.h>`
- `length < 2`, because a single point defines no interval to interpolate across
- an `xTable` that is not **strictly** ascending

Strictly, not merely non-decreasing. Two equal x entries make the divisor zero, and that is the defect this check exists to stop. The scan is O(N) and is paid once.

## interpCalculate

```
x <= xTable[0]            ->  yTable[0]
x >= xTable[length - 1]   ->  yTable[length - 1]
otherwise                 ->  interpolate
```

The interpolating branch finds the first index whose x is strictly greater than the input — the shape of `searchUpperBound` — and brackets on the one before it:

```c
y = yTable[i] + ( x - xTable[i] ) * ( yTable[i + 1] - yTable[i] )
                                  / ( xTable[i + 1] - xTable[i] );
```

Two invariants make this safe and neither is checked at run time. The divisor is never zero because `Init` proved strict ascent. The index `i` is always in `[0, length - 2]` because the clamps run first, so the interpolating branch is only reached when `xTable[0] < x < xTable[length - 1]`, which puts the upper-bound result in `[1, length - 1]`.

The bracketing search is the module's own code. `interp.c` cannot include `search.h` — module independence forbids it, and that independence is what makes single-module copy-out work. The duplication is deliberate, and the source banner says so, so the next reader does not "fix" it into a cross-module include.

### The i32 arithmetic

Every intermediate is `int64_t`:

```c
num = ( ( int64_t ) x - ( int64_t ) xTable[i] ) *
      ( ( int64_t ) yTable[i + 1] - ( int64_t ) yTable[i] );
den = ( int64_t ) xTable[i + 1] - ( int64_t ) xTable[i];
```

The subtractions are widened too, not only the product. The bracket is narrow, but the table's own span can fill `int32_t`, and a difference taken in `int32_t` first would overflow before the widening could help.

`int64_t` is new to this tree — nothing under `src/` or `drv/` uses it today. It is not new to the language the library already targets: it comes from the `<stdint.h>` every header includes, and `arm-none-eabi-gcc` provides it on every part this library is built for. On a 32-bit core a 64-bit multiply and divide are library calls rather than instructions, which is the honest cost of this variant and is worth a `@note`.

The division rounds to nearest rather than truncating toward zero. `den` is always positive because the table ascends strictly, so the rounding only has to follow the sign of `num`:

```c
if ( num >= 0 ) { num += den / 2; } else { num -= den / 2; }
```

Truncation would bias every result toward the lower node and double the worst-case error of an integer calibration table. The final sum cannot overflow: the quotient's magnitude is bounded by `|yTable[i + 1] - yTable[i]|`, so the result stays between the two nodes.

## interpInRange

`TRUE` when `xTable[0] <= x <= xTable[length - 1]`, endpoints included.

It exists because `interpCalculate` returns a value in every case — clamping is an answer, not an error, which is why the value is returned directly the way `mathFindMax` returns one, rather than through an out-parameter behind a status. But an input past the end of the table sometimes means a broken sensor rather than a saturated one, and the caller who needs to tell the difference asks separately. The two questions stay independent and both stay cheap.

## Testing

`test/Interp_Test/Interp_Test.c`, assert style, no `output.txt`, non-zero exit on failure — the house style for new tests.

| Case | What it proves |
|---|---|
| `Init` rejection | NULL driver and either table, `length` of 0 and 1, a descending table, and a table with a duplicate x. The sentinel pattern confirms the driver is left untouched on `FALSE` |
| Node values | An input exactly at `xTable[i]` returns exactly `yTable[i]`, at both ends and in the middle |
| Midpoint | Halfway between two nodes gives the mean of their y values |
| Clamping | Below the first x and above the last, both widths |
| Descending y | Ascending x with descending y, which is what an NTC curve is. Catches a sign error in the slope |
| `interpInRange` | At both endpoints, inside, and outside on each side |
| i32 rounding | A table where the exact answer falls on `.5`, so truncation and round-to-nearest give different integers |

The pinned regression is the duplicate x. If `Init` accepted it, the i32 path would divide by zero and the float path would produce `inf` or `nan` and hand it straight to the caller, since nothing downstream limits the output. Every other case in the file passes with that bug present, so the rejection branch is the only thing that catches it.

## Verification

- the tree-wide `-Wall -Wextra` sweep stays silent
- every header still coexists in one translation unit
- all six new exported symbols are referenced by the test
- exported symbol count 192 to 198
- test program count 21 to 22
- the assert-style test list in CLAUDE.md grows from fourteen to fifteen

## CLAUDE.md

- a paragraph in the module tour: `searchClosest` says which entry to read, `interp` gives the value between two entries, and the two are the halves of the same job
- `interp` added to the list of stateful modules that use their own name as a prefix
- the symbol, test and assert-style counts above
- a row in the pinned-bug table for the duplicate x
