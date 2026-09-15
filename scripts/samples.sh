#!/bin/sh
#
# Build and run every example under sample/.
#
# Run from the repository root:
#     sh scripts/samples.sh
#     sh scripts/samples.sh MotionLoop     one of them
#
# The samples are documentation, and documentation that does not compile is
# worse than none. Nothing checked them until 15/09/2026, and one of them —
# sample/DFT — had not built for years: it called time() without <time.h>,
# which a modern compiler makes an error rather than a warning. That is the
# same defect WriteToAFile.c carried until the test suite was first run, found
# the same way, by finally running the thing.
#
# Library dependencies are derived from each sample's own #include "..." lines,
# exactly as run_tests.sh does for the tests, so adding a sample needs no edit
# here. A sample that includes nothing from the tree builds on its own, which
# is what the older ones under sample/ do — they teach C rather than this
# library.
#
# Environment:
#     CC       compiler to use, default gcc
#     CFLAGS   extra flags, default empty
#     QUIET    set to 1 to build without running
#
# Exit status is the number of samples that failed.

CC=${CC:-gcc}
CFLAGS=${CFLAGS:-}
QUIET=${QUIET:-0}
ONLY=${1:-}

if [ ! -d sample ] || [ ! -d inc ]; then
    echo "run from the repository root (no sample/ or inc/ here)" >&2
    exit 126
fi

outdir=$(mktemp -d 2>/dev/null || echo /tmp/esclib_samples.$$)
mkdir -p "$outdir"

built=0
buildfail=0
runfail=0

for d in sample/*/; do
    name=$(basename "$d")

    if [ -n "$ONLY" ] && [ "$name" != "$ONLY" ]; then
        continue
    fi

    main=$(ls "$d"*.c 2>/dev/null | head -1)

    if [ -z "$main" ]; then
        continue
    fi

    srcs=""
    idirs=""
    missing=""

    for h in $(grep -h '^#include "' "$d"*.c 2>/dev/null | sed 's/.*"\(.*\)".*/\1/' | sort -u); do
        hpath=$(find inc drv -name "$h" 2>/dev/null | head -1)

        if [ -z "$hpath" ]; then
            missing="$missing $h"
            continue
        fi

        idir=$(dirname "$hpath")
        case " $idirs " in
            *" -I$idir "*) ;;
            *) idirs="$idirs -I$idir" ;;
        esac

        case "$hpath" in
            inc/*) csrc="src/${hpath#inc/}" ;;
            *)     csrc="$hpath" ;;
        esac
        csrc="${csrc%.h}.c"

        if [ ! -f "$csrc" ]; then
            missing="$missing $csrc"
            continue
        fi

        srcs="$srcs $csrc"
    done

    if [ -n "$missing" ]; then
        echo "SKIP  $name — unresolved:$missing"
        continue
    fi

    bin="$outdir/$name"

    if $CC -Wall -Wextra $CFLAGS $idirs -o "$bin" "$main" $srcs -lm 2>"$outdir/$name.build"; then
        built=$((built + 1))
    else
        echo "BUILD FAIL  $name"
        sed 's/^/    /' "$outdir/$name.build"
        buildfail=$((buildfail + 1))
        continue
    fi

    # A warning in a sample is a regression here for the reason it is one in
    # src/: somebody is going to copy this.
    if [ -s "$outdir/$name.build" ]; then
        echo "WARNINGS  $name"
        sed 's/^/    /' "$outdir/$name.build"
        buildfail=$((buildfail + 1))
        continue
    fi

    [ "$QUIET" = "1" ] && { echo "BUILT $name"; continue; }

    # FileWriteLog and the older samples write into the working directory, so
    # they are run somewhere disposable.
    if ( cd "$outdir" && "$bin" >"$outdir/$name.out" 2>&1 ); then
        echo "RUN   $name"
    else
        status=$?
        echo "RUN FAIL  $name (exit $status)"
        sed 's/^/    /' "$outdir/$name.out" | head -10
        runfail=$((runfail + 1))
    fi
done

echo
echo "built $built, build failures $buildfail, run failures $runfail"
echo "artifacts in $outdir"

total=$((buildfail + runfail))
[ "$total" -gt 125 ] && total=125
exit "$total"
