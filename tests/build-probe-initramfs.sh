#!/bin/bash
set -euo pipefail
mode=${1:?baseline or candidate required}
case "$mode" in baseline|candidate) ;; *) exit 2;; esac
root="/work/tests/root-with-probe-$mode"
mkdir -p "$root"
cp -a /work/tests/root/. "$root/"
cp /work/tests/init "$root/init"
chmod 0755 "$root/init"
cp "/work/tests/$mode/rcu_lifetime_probe.ko" "$root/tests/rcu_lifetime_probe.ko"
if [ "$mode" = candidate ]; then
    cp /work/tests/candidate/dummy.ko "$root/tests/dummy.ko"
fi
cd "$root"
find . -print0 | cpio --null --create --format=newc --owner=0:0 2>"/work/logs/$mode-probe-initramfs-cpio.log" | gzip -1 > "/work/tests/$mode-probe-initramfs.cpio.gz"
sha256sum "/work/tests/$mode-probe-initramfs.cpio.gz" > "/work/evidence/$mode-probe-initramfs-sha256.txt"
