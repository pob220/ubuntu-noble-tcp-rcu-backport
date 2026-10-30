#!/bin/bash
set -euo pipefail
mode=${1:?baseline or candidate}
case "$mode" in
  baseline) image=/work/tests/baseline/root/boot/vmlinuz-6.8.0-147-generic ;;
  candidate) image=/work/artifacts/extracted/boot/vmlinuz-6.8.0-147.1+ionostcp1-generic ;;
  *) exit 2 ;;
esac
disk="/work/tests/$mode-efi.img"
test -f "$image"
test -f "/work/tests/$mode-probe-initramfs.cpio.gz"
cat > "/work/tests/$mode-grub.cfg" <<'EOF'
serial --unit=0 --speed=115200
terminal_input serial
terminal_output serial
set timeout=0
search --no-floppy --label KERNELTEST --set=root
menuentry 'Isolated kernel regression test' {
    linux /vmlinuz console=ttyS0 rdinit=/init panic=-1 oops=panic
    initrd /test-initramfs.cpio.gz
}
EOF
if [ "$mode" = candidate ]; then
    sed -i 's/oops=panic/oops=panic module.sig_enforce=1/' "/work/tests/$mode-grub.cfg"
fi
grub-mkstandalone -O x86_64-efi -o "/work/tests/$mode-BOOTX64.EFI" \
    --modules='part_gpt fat normal linux serial terminal search search_label' \
    "boot/grub/grub.cfg=/work/tests/$mode-grub.cfg"
truncate -s 256M "$disk"
parted --script "$disk" mklabel gpt mkpart EFI fat32 1MiB 255MiB set 1 esp on
mformat -i "$disk@@1048576" -F -T 520192 -v KERNELTEST ::
mmd -i "$disk@@1048576" ::/EFI ::/EFI/BOOT
mcopy -i "$disk@@1048576" "/work/tests/$mode-BOOTX64.EFI" ::/EFI/BOOT/BOOTX64.EFI
mcopy -i "$disk@@1048576" "$image" ::/vmlinuz
mcopy -i "$disk@@1048576" "/work/tests/$mode-probe-initramfs.cpio.gz" ::/test-initramfs.cpio.gz
sha256sum "$disk" > "/work/evidence/$mode-efi-image-sha256.txt"
