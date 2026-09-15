#!/bin/sh
#
# Code size of every esclib module, per module.
#
# Run from the repository root:
#     sh scripts/size.sh
#
# The documentation in this tree argues about cost constantly — that a 256 byte
# table is the wrong trade for an 8 bit CRC, that evaluating a cosine at boot
# links the whole software float library, that maf stays O(1) where fir is
# O(N). Those arguments were all asserted and none of them was ever measured.
# This measures them. It is also how a module that quietly doubled in size gets
# noticed, which nothing else here would catch.
#
# It builds nothing that ships and it is not a gate: the numbers are for a
# human to read. scripts/check.sh is where a rule gets enforced, and the one
# rule these numbers carry — that data and bss must both be zero, because the
# caller owns all storage — is checked there.
#
# Environment:
#     CC       compiler, default arm-none-eabi-gcc
#     CFLAGS   optimization and target flags, default -Os for a Cortex-M0.
#              The defaults are the smallest part this library is aimed at, so
#              the numbers are the worst realistic case for code size.
#     SIZE     section sizer, default derived from CC
#
# Exit status is 0 unless the tree could not be built.

CC=${CC:-arm-none-eabi-gcc}
CFLAGS=${CFLAGS:--Os -mcpu=cortex-m0 -mthumb}

if [ ! -d inc ] || [ ! -d src ]; then
    echo "run from the repository root (no inc/ or src/ here)" >&2
    exit 126
fi

if ! command -v "$CC" >/dev/null 2>&1; then
    echo "$CC not found. Set CC to a compiler you have." >&2
    exit 127
fi

if [ -z "$SIZE" ]; then
    SIZE=$(printf '%s' "$CC" | sed 's/gcc$/size/')
    command -v "$SIZE" >/dev/null 2>&1 || SIZE=size
fi

outdir=$(mktemp -d 2>/dev/null || echo /tmp/esclib_size.$$)
mkdir -p "$outdir"

echo "using CC=$CC CFLAGS=$CFLAGS"
echo

failures=0

for f in src/*/*.c drv/*.c; do
    [ -f "$f" ] || continue

    m=$(basename "$(dirname "$f")")
    inc="inc/$m"
    [ -d "$inc" ] || inc="drv"

    name=$(basename "$f" .c)
    obj="$outdir/$name.o"

    if ! $CC -c $CFLAGS -I"$inc" -Idrv "$f" -o "$obj" 2>"$outdir/cc.err"; then
        echo "BUILD FAIL  $f"
        sed 's/^/    /' "$outdir/cc.err"
        failures=$((failures + 1))
        continue
    fi

    # text data bss dec hex filename
    line=$($SIZE "$obj" 2>/dev/null | tail -1)
    printf '%s %s\n' "$name" "$line" >> "$outdir/rows"
done

if [ ! -f "$outdir/rows" ]; then
    echo "nothing built" >&2
    exit 1
fi

printf '%-16s %6s %6s %6s\n' module text data bss
printf '%-16s %6s %6s %6s\n' ---------------- ------ ------ ------

sort -k2 -rn "$outdir/rows" | awk '{ printf "%-16s %6d %6d %6d\n", $1, $2, $3, $4 }'

printf '%-16s %6s %6s %6s\n' ---------------- ------ ------ ------

awk '{ t += $2; d += $3; b += $4; n += 1 }
     END { printf "%-16s %6d %6d %6d   (%d modules)\n", "total", t, d, b, n }' "$outdir/rows"

echo
echo "text includes read only tables, which is where crc16 and crc32 carry theirs."
echo "data and bss must both be zero: the caller owns all storage, and"
echo "scripts/check.sh fails the tree when one of them is not."

echo
echo "objects in $outdir"

exit "$failures"
