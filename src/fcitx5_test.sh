#!/usr/bin/env bash

set -eu

want=$(printf "1\r\n0\r")

raw=$(./vim -u NONE -e --cmd 'call ch_logfile("test.log", "w") | call fcitx5_activate(1) | echo fcitx5_status() | call fcitx5_activate(0) | echo fcitx5_status()| q' 2>&1)

got=$(echo "$raw" | sed -E "s/\\x1B\\[\\??([0-9]{1,2}(;[0-9]{1,2})?)?[A-Za-z]//g" )

if [ "$want" != "$got" ] ; then
  echo "not match: got..."
  printf "%q\n" "$got"
  exit 1
fi
