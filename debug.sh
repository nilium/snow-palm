#!/bin/sh

TMPFILE=`mktemp /tmp/breakpoints.XXXXXXXX` || exit 1

echo "break s_fatal_error_impl" > "$TMPFILE"
for BP in `grep --exclude='*.h' -Erone '(S_BREAKPOINT|s_(fatal_error|log_error)\()' src`
do
  BP=${BP##*/}
  BP=${BP%:*}
  echo "break $BP" >> "$TMPFILE"
done

gdb \
  -q \
  bin/snow \
  -x "$TMPFILE"

rm "$TMPFILE"