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
# There are two kinds of reference and this used to report them as one, which
# was found on 16/09/2026 rather than designed in:
#
#   calls        an instruction in the object branches to the helper. This is
#                read from the relocations, so it is what the code does.
#   links only   the object names the helper in its symbol table and no
#                instruction reaches it. gcc declares a libcall while it is
#                weighing an expansion and keeps the declaration after throwing
#                the code away, so this happens without anything in the source
#                asking for it — text names a signed divide and divides nothing
#                signed. It is not free either: an undefined symbol pulls its
#                archive member into a plain link, and only --gc-sections drops
#                it again.
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
# This is a report, not a gate, with one exception: no module may need a double
# precision helper, and that rule counts both kinds of reference, because a
# double that reached even a discarded expansion came from somewhere.
# scripts/check.sh is where the rest of the rules are enforced.
#
# Environment:
#     CC       compiler, default arm-none-eabi-gcc
#     CFLAGS   target flags, default -Os for a Cortex-M0, which is the part
#              with the fewest instructions and so the most helpers
#     NM       symbol lister, default derived from CC
#     OBJDUMP  disassembler, default derived from CC
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

if [ -z "$OBJDUMP" ]; then
    OBJDUMP=$(printf '%s' "$CC" | sed 's/gcc$/objdump/')
    command -v "$OBJDUMP" >/dev/null 2>&1 || OBJDUMP=objdump
fi

outdir=$(mktemp -d 2>/dev/null || echo /tmp/esclib_runtime.$$)
mkdir -p "$outdir"

echo "using CC=$CC CFLAGS=$CFLAGS"
echo

failures=0
doubles=0
listedonly=0

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

    # What the code branches to, taken from the relocations.
    $OBJDUMP -dr "$obj" 2>/dev/null \
        | awk '$2 ~ /^R_/ { print $NF }' \
        | grep -oE '__aeabi_[a-z0-9]+' | sort -u > "$outdir/called.txt"

    # What the object names, which is what the linker acts on.
    $NM -u "$obj" 2>/dev/null \
        | grep -oE '__aeabi_[a-z0-9]+' | sort -u > "$outdir/named.txt"

    called=$(tr '\n' ' ' < "$outdir/called.txt")
    only=$(grep -vxF -f "$outdir/called.txt" "$outdir/named.txt" 2>/dev/null \
           | tr '\n' ' ')

    if [ -n "$called" ]; then
        printf '%-16s %s\n' "$name" "$called"
    else
        printf '%-16s %s\n' "$name" "none"
    fi

    if [ -n "$only" ]; then
        printf '%-16s links only: %s\n' "" "$only"
        listedonly=$((listedonly + 1))
    fi

    case "$called$only" in
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

if [ "$listedonly" -gt 0 ]; then
    echo
    echo "$listedonly module(s) name a helper that no instruction reaches."
    echo "Nothing in the source asked for those: gcc declares a libcall while"
    echo "weighing an expansion and keeps the declaration after discarding the"
    echo "code. A plain link still pulls each one in; --gc-sections drops them."
fi

echo
echo "objects in $outdir"

exit "$failures"
