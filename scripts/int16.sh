#!/bin/sh
#
# esclib on a part whose int is sixteen bits wide.
#
# Run from the repository root:
#     sh scripts/int16.sh
#
# Every other compiler in reach has a 32 bit int, and C's integer promotions
# are defined in terms of int. So a whole class of defect is invisible on all
# of them at once: two int16_t multiply in int and overflow, a uint8_t shifted
# by sixteen is shifted past the width of the type it was promoted to, and
# every test passes because the host did the arithmetic in thirty-two bits.
# The library claims C99 and freestanding, and an 8 bit or 16 bit part with a
# 16 bit int is as freestanding as it gets. This builds the tree for one.
#
# Two checks, and a probe in front of them:
#
#   - **Warnings.** Every module under -Wall -Wextra for an AVR. A shift by a
#     constant past the width of int is a warning gcc gives on its own, so a
#     byte shifted into the top of a 32 bit word without a cast first fails
#     here and nowhere else. The strict profile is not applied: under the gcc
#     7 that ships for AVR it reports eight things, and all eight belong to
#     the toolchain rather than to int — avr-libc maps cosf and friends onto
#     its double functions, and gcc 7 warns on narrow compound assignments
#     gcc 13 has since stopped warning on.
#   - **Pins.** An overflow at run time is not a warning, so where the
#     library's correctness rests on widening before it multiplies, the
#     widening is checked in the code the compiler emitted. The pins below
#     name a function and the runtime helper its object has to call; the
#     relocations against that function's own section say whether it does.
#     A pin is a statement about this compiler's output, which is why the
#     probe checks the method before the pins are trusted.
#
# The probe builds three small functions and requires that the shift past
# int's width warns, that the widened multiply calls the helper and that the
# narrow one does not. If any of the three goes the other way, this compiler
# does not behave the way the pins assume and the result for the tree would
# mean nothing.
#
# Execution under a simulator was considered and left out: the tests need far
# more RAM than an ATmega328P has, and nothing that runs on this machine would
# check it. A pin read from the object is narrower, and it is checked here.
#
# Environment:
#     INT16_CC       compiler, default avr-gcc. Any gcc whose int is 16 bits
#                    will do for the warnings; the pins name AVR helpers.
#     INT16_CFLAGS   default -Os -mmcu=atmega328p, the commonest such part.
#     INT16_READELF  default derived from INT16_CC.
#
# The names are this script's own rather than CC and CFLAGS on purpose.
# scripts/mutate.sh passes CC through to whatever it runs, and a host CC
# arriving here would build for a 32 bit int.
#
# Exit status is the number of checks that failed. It is 127 when the compiler
# is missing and 126 when the probe shows it is not one these checks can be
# made with — and scripts/mutate.sh counts both as a mutation it could not
# apply, never as one that was caught.

CC=${INT16_CC:-avr-gcc}
CFLAGS=${INT16_CFLAGS:--Os -mmcu=atmega328p}
READELF=${INT16_READELF:-}

if [ ! -d inc ] || [ ! -d src ]; then
    echo "run from the repository root (no inc/ or src/ here)" >&2
    exit 126
fi

if ! command -v "$CC" >/dev/null 2>&1; then
    echo "$CC not found. Set CC to a compiler whose int is 16 bits." >&2
    exit 127
fi

if [ -z "$READELF" ]; then
    READELF=$(printf '%s' "$CC" | sed 's/gcc$/readelf/')
    command -v "$READELF" >/dev/null 2>&1 || READELF=readelf
fi

# The pins. One per line: source file, function, the helper it must call, and
# why. Adding one is a line here and a mutation under scripts/mutations/ that
# names this script as its TEST.
PINS='src/filter/rms.c rmsIterationi16 __mulhisi3 the square of an int16_t sample is formed in 32 bits'

outdir=$(mktemp -d 2>/dev/null || echo /tmp/esclib_int16.$$)
mkdir -p "$outdir"

echo "using CC=$CC CFLAGS=$CFLAGS"
echo

failures=0

# Prints the runtime helpers a function's own section is relocated against.
# -ffunction-sections gives every function a section of its own, so the
# relocation table for that section is exactly what that function calls.
helpersOf ( ) {
    "$READELF" -rW "$1" 2>/dev/null | awk -v section=".rela.text.$2" '
        /^Relocation section/ {
            inside = ( index ( $0, "'"'"'" section "'"'"'" ) > 0 )
            next
        }
        inside && NF >= 5 { print $5 }
    '
}

# ---------------------------------------------------------------- the probe

cat > "$outdir/probe.c" <<'EOF'
#include <stdint.h>

int32_t probeWide ( int16_t x )
{
    return ( ( int32_t ) x ) * ( ( int32_t ) x );
}

int32_t probeNarrow ( int16_t x )
{
    return x * x;
}

uint32_t probeShift ( uint8_t b )
{
    return b << 24;
}
EOF

probeOk=1

if ! $CC -c $CFLAGS -ffunction-sections -Wall -Wextra "$outdir/probe.c" \
        -o "$outdir/probe.o" 2>"$outdir/probe.err"; then
    sed 's/^/    /' "$outdir/probe.err"
    probeOk=0
else
    grep -q 'shift count >= width' "$outdir/probe.err" || probeOk=0
    helpersOf "$outdir/probe.o" probeWide | grep -q '^__mulhisi3$' || probeOk=0
    helpersOf "$outdir/probe.o" probeNarrow | grep -q '^__mulhisi3$' && probeOk=0
fi

if [ "$probeOk" -eq 1 ]; then
    echo "OK    the probe: int is 16 bits, a shift past it warns, and a widened"
    echo "      multiply is told from a narrow one by the helper it calls"
else
    # Not a failure of the tree: this compiler cannot answer the question.
    echo "NOT RUN  the probe did not behave as the checks assume — int is not"
    echo "         16 bits here, or the helpers have other names — so the tree"
    echo "         was not checked"
    exit 126
fi

# ---------------------------------------------------------------- warnings

echo
echo "== warnings =="

files=0
warned=0

for f in src/*/*.c drv/*.c; do
    [ -f "$f" ] || continue

    m=$(basename "$(dirname "$f")")
    inc="inc/$m"
    [ -d "$inc" ] || inc="drv"

    files=$((files + 1))

    if ! $CC -c $CFLAGS -Wall -Wextra -I"$inc" -Idrv "$f" \
            -o "$outdir/$(basename "$f" .c).o" 2>"$outdir/cc.err"; then
        echo "BUILD FAIL  $f"
        sed 's/^/    /' "$outdir/cc.err"
        warned=$((warned + 1))
    elif [ -s "$outdir/cc.err" ]; then
        echo "WARN  $f"
        sed 's/^/    /' "$outdir/cc.err"
        warned=$((warned + 1))
    fi
done

if [ "$warned" -eq 0 ]; then
    echo "OK    $files files, no warnings with a 16 bit int"
else
    echo "FAIL  $warned of $files files"
    failures=$((failures + 1))
fi

# ---------------------------------------------------------------- pins

echo
echo "== pins =="

printf '%s\n' "$PINS" > "$outdir/pins"

while read -r file function helper why; do
    [ -n "$file" ] || continue

    m=$(basename "$(dirname "$file")")
    obj="$outdir/pin_$function.o"

    if ! $CC -c $CFLAGS -ffunction-sections -I"inc/$m" "$file" -o "$obj" \
            2>"$outdir/cc.err"; then
        echo "BUILD FAIL  $file"
        sed 's/^/    /' "$outdir/cc.err"
        failures=$((failures + 1))
    elif helpersOf "$obj" "$function" | grep -q "^$helper\$"; then
        echo "OK    $function calls $helper: $why"
    else
        echo "FAIL  $function no longer calls $helper, so $why no longer holds"
        failures=$((failures + 1))
    fi
done < "$outdir/pins"

echo
echo "objects in $outdir"

exit "$failures"
