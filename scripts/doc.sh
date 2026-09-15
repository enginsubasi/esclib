#!/bin/sh
#
# Generate the esclib API reference with Doxygen.
#
# Run from the repository root:
#     sh scripts/doc.sh
#
# Output goes to doc/, which is ignored by git on purpose. The reference is
# generated from the sources when somebody wants it, not checked in: a
# committed HTML tree is stale the moment a banner changes and bloats every
# clone of a library whose whole point is that a module is one .h and one .c.
#
# Environment:
#     DOXYGEN  doxygen executable, default doxygen
#
# Exit status is doxygen's, or 127 when doxygen is not installed.

DOXYGEN=${DOXYGEN:-doxygen}

if [ ! -f Doxyfile ]; then
    echo "run from the repository root (no Doxyfile here)" >&2
    exit 126
fi

if ! command -v "$DOXYGEN" >/dev/null 2>&1; then
    echo "$DOXYGEN not found. Install Doxygen or set DOXYGEN to its path." >&2
    exit 127
fi

"$DOXYGEN" Doxyfile
status=$?

if [ "$status" -eq 0 ]; then
    echo
    echo "generated doc/html/index.html"
fi

exit "$status"
