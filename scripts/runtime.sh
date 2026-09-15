#!/bin/sh
#
# Which compiler runtime helpers each esclib module pulls in.
#
# Run from the repository root:
#     sh scripts/runtime.sh
#
# A module that uses float on a part without an FPU links the software float
# routines, and everyone knows that. What is less obvious, and what this
# reports, is that the fixed point variants written to avoid them pull in
# 64-bit integer helpers instead: a Q16 multiply needs __aeabi_lmul and a Q16
# divide needs __aeabi_ldivmod, neither of which a Cortex-M0 has an instruction
# for. That is the right trade — an int64_t intermediate is what keeps those
# variants correct — but the cost is real and belongs next to the claim.
#
# Reading the output:
#     __aeabi_f*          software single precision float
#     __aeabi_d*          software double precision, which nothing here should
#                         need; one appearing is a bug
#     __aeabi_lmul        64-bit multiply
#     __aeabi_ldivmod     64-bit signed divide
#     __aeabi_uldivmod    64-bit unsigned divide
#     __aeabi_idiv(mod)   32-bit divide, which a Cortex-M0 also lacks
#
# This is a report, not a gate. scripts/check.sh is where a rule is enforced.
#
# Environment:
#     CC       compiler, default arm-none-eabi-gcc
#     CFLAGS   target flags, default -Os for a Cortex-M0, which is the part
#              with the fewest instructions and so the most helpers
#     NM       symbol lister, default derived from CC
#
# Exit status is the number of modules that failed to build.

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

if [ -z "$NM" ]; then
    NM=$(printf '%s' "$CC" | sed 's/gcc$/nm/')
    command -v "$NM" >/dev/null 2>&1 || NM=nm
fi

outdir=$(mktemp -d 2>/dev/null || echo /tmp/esclib_runtime.$$)
mkdir -p "$outdir"

echo "using CC=$CC CFLAGS=$CFLAGS"
echo

failures=0
doubles=0

printf '%-16s %s\n' module "runtime helpers"
printf '%-16s %s\n' ---------------- ------------------------------------------------

for f in src/*/*.c drv/*.c; do
    [ -f "$f" ] || continue

    m=$(basename "$(dirname "$f")")
    inc="inc/$m"
    [ -d "$inc" ] || inc="drv"

    name=$(basename "$f" .c)
    obj="$outdir/$name.o"

    if ! $CC -c $CFLAGS -I"$inc" -Idrv "$f" -o "$obj" 2>"$outdir/cc.err"; then
        echo "BUILD FAIL  $f"
        failures=$((failures + 1))
        continue
    fi

    helpers=$($NM -u "$obj" 2>/dev/null | grep -oE '__aeabi_[a-z0-9]+' | sort -u | tr '\n' ' ')

    if [ -n "$helpers" ]; then
        printf '%-16s %s\n' "$name" "$helpers"
    else
        printf '%-16s %s\n' "$name" "none"
    fi

    case "$helpers" in
        *__aeabi_d*)
            doubles=$((doubles + 1))
            ;;
        *) ;;
    esac
done

echo

if [ "$doubles" -eq 0 ]; then
    echo "no module needs a double precision helper, which is the one result"
    echo "here that is a rule rather than an observation: this library is"
    echo "single precision throughout and a __aeabi_d* appearing above means"
    echo "a double slipped into an expression somewhere."
else
    echo "$doubles module(s) pull in double precision helpers — see above."
    failures=$((failures + doubles))
fi

echo
echo "objects in $outdir"

exit "$failures"
