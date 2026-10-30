#!/bin/bash
set -euo pipefail
base=/work/tests
mkdir -p "$base/root/bin" "$base/root/tests" "$base/root/proc" "$base/root/sys" "$base/root/dev" "$base/root/tmp"
cp /bin/busybox "$base/root/bin/busybox"
for test in reuseport_addr_any reuseport_dualstack reuseaddr_conflict reuseport_bpf; do
    gcc -static -O2 -Wall -Wextra -o "$base/root/tests/$test" \
      "/work/source/linux-6.8.0/tools/testing/selftests/net/$test.c" -pthread
done
gcc -static -O2 -Wall -Wextra -Werror -o "$base/root/tests/tcp_options_control" "$base/tcp_options_control.c" -pthread
gcc -static -O2 -Wall -Wextra -Werror -o "$base/root/tests/socket_edge_controls" "$base/socket_edge_controls.c"
cp "$base/init" "$base/root/init"
chmod 0755 "$base/root/init"
cd "$base/root"
find . -print0 | cpio --null --create --format=newc --owner=0:0 2>/work/logs/test-initramfs-cpio.log | gzip -1 > "$base/test-initramfs.cpio.gz"
sha256sum "$base/test-initramfs.cpio.gz" > /work/evidence/test-initramfs-sha256.txt
