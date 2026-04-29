#!/usr/bin/env sh
set -eu

BIN=$1
OUT=$("$BIN" 2>&1) && STATUS=0 || STATUS=$?

if [ "$STATUS" -ne 2 ]; then
    echo "expected exit code 2, got $STATUS" >&2
    echo "$OUT" >&2
    exit 1
fi

case "$OUT" in
    *"Usage:"*) exit 0 ;;
    *)
        echo "expected Usage output" >&2
        echo "$OUT" >&2
        exit 1
        ;;
esac
