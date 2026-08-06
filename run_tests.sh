#!/bin/sh
#
# Build and run every esclib test with a host compiler.
#
# Run from the repository root:
#     sh run_tests.sh
#
# This is the one exception to the rule that esclib has no build system. It
# builds nothing that ships: the library is still consumed by copying module
# source pairs into a target project, and nothing here produces an artifact.
# What it does is run the tests, which until 05/08/2026 had never been run at
# all, and it earns its place by needing no maintenance — each test's module
# dependencies are derived from its own #include lines, so there is no list to
# keep in sync with the tree. Adding a module and its test requires no edit
# here.
#
# Environment:
#     CC       compiler to use, default gcc
#     CFLAGS   extra flags, default empty
#     LINKONLY set to 1 to compile and link without running. Use this with a
#              cross compiler to check the source sets are complete:
#                  CC=arm-none-eabi-gcc CFLAGS=--specs=nosys.specs LINKONLY=1 sh run_tests.sh
#
# Exit status is the number of tests that failed, capped at 125.

CC=${CC:-gcc}
CFLAGS=${CFLAGS:-}
LINKONLY=${LINKONLY:-0}

if [ ! -d inc ] || [ ! -d test ]; then
    echo "run from the repository root (no inc/ or test/ here)" >&2
    exit 126
fi

outdir=$(mktemp -d 2>/dev/null || echo /tmp/esclib_tests.$$)
mkdir -p "$outdir"

built=0
buildfail=0
ran=0
runfail=0
skipped=""

for d in test/*/; do
    name=$(basename "$d")
    main=$(ls "$d"*.c 2>/dev/null | head -1)

    if [ -z "$main" ]; then
        skipped="$skipped $name(no .c)"
        continue
    fi

    # Headers this test includes, resolved to their module sources.
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

        # inc/<module>/<name>.h -> src/<module>/<name>.c ; drv/<name>.h -> drv/<name>.c
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
        skipped="$skipped $name"
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

    # A warning is a regression in this tree, so surface it even when the build
    # succeeded. Newlib syscall stub warnings from a cross link are not ours.
    if [ -s "$outdir/$name.build" ] && ! grep -q "is not implemented" "$outdir/$name.build"; then
        echo "WARNINGS  $name"
        sed 's/^/    /' "$outdir/$name.build"
    fi

    [ "$LINKONLY" = "1" ] && continue

    if "$bin" >"$outdir/$name.out" 2>&1; then
        ran=$((ran + 1))
        echo "PASS  $name"
    else
        status=$?
        runfail=$((runfail + 1))
        echo "FAIL  $name (exit $status)"
        sed 's/^/    /' "$outdir/$name.out"
    fi

    # Four printing tests keep a checked-in output.txt holding their stdout.
    # A difference is reported, never counted as a failure: regenerating one is
    # a judgement call about whether the module or the expectation moved.
    #
    # WriteToAFile_Test is excluded. Its output.txt is the file the program
    # writes, not its stdout, so comparing the two is a category error.
    #
    # The CRs are stripped on both sides. The sources print \r\n and a Windows
    # text mode stream adds another \r, so a raw redirect stores \r\r\n while
    # the checked-in files carry plain LF. Written with temporary files rather
    # than process substitution, which is a bashism this script avoids.
    if [ -f "$d/output.txt" ] && [ "$name" != "WriteToAFile_Test" ]; then
        tr -d '\r' < "$d/output.txt" > "$outdir/$name.want"
        tr -d '\r' < "$outdir/$name.out" > "$outdir/$name.got"

        if ! diff -q "$outdir/$name.want" "$outdir/$name.got" >/dev/null 2>&1; then
            echo "      output.txt differs — regenerate with:"
            echo "      $bin | tr -d '\\r' > $d/output.txt"
        fi
    fi
done

# WriteToAFile_Test writes an output.txt into whatever directory it runs in,
# which is the repository root. Left behind it shows up as an untracked file
# and eventually gets committed by accident.
if [ -f output.txt ]; then
    rm -f output.txt
fi

echo
echo "built $built, build failures $buildfail"
if [ "$LINKONLY" = "1" ]; then
    echo "link-only mode, nothing executed"
else
    echo "ran $ran, run failures $runfail"
fi
[ -n "$skipped" ] && echo "skipped:$skipped"
echo "artifacts in $outdir"

total=$((buildfail + runfail))
[ "$total" -gt 125 ] && total=125
exit "$total"
