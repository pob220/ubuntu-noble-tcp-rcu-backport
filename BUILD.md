# Build and validation record

Use a disposable Ubuntu 24.04 container. The original environment used a rootless Podman builder pinned to the image digest in `Containerfile`, limited to 12 CPUs and 28 GiB RAM. `build-environment-versions.txt` records the actual package versions. The Containerfile uses the current authenticated archive for dependencies; a future rebuild may need Canonical's snapshot service to obtain superseded exact versions. Identical binary hashes are not promised: timestamps, module signing keys and dependency versions can differ.

## Source authentication and native build

Place all release downloads in `downloads/`. Start by checking `SHA256SUMS`. Build the disposable environment:

```sh
podman build -t localhost/noble-tcp-rcu-builder .
podman run --rm --userns=keep-id --cpus=12 --memory=28g \
  -v "$PWD:/work" -w /work localhost/noble-tcp-rcu-builder bash build.sh
```

The build verifies `InRelease -> Sources -> source files` with the official Ubuntu keyring, reconstructs Ubuntu's source permissions/symlinks, applies the two patches without fuzz, runs native `clean`, caches checksum-pinned ancillary sources and runs:

```sh
fakeroot debian/rules -j12 binary-headers binary-generic do_tools=false
```

Native configuration, module and signature checks remain enabled. `do_tools=false` suppresses unrelated user-space tools. The native rules can generate optional additional packages; `build.sh` selects only the exact five kernel/header/module packages. It performs no host installation. Keep generated signing keys private; the builder generates its own key, so the published binaries' signatures cannot be reproduced without that private key.

`verify-source.py` accepts `--keyring` for a separately trusted Ubuntu archive keyring. The old `.dsc` uploader signature was not independently trusted by the original build environment; the `.dsc` and its contents were instead authenticated through the signed Ubuntu archive/Sources checksum chain. Verification of the archived release signature and checksums passed independently with `gpgv`.

## VM regression harness

The published C test and init scripts are the ones compiled and run for the recorded result. They are intended for an isolated lab, not a production host. In a disposable build container:

1. Extract the exact stock147 image/modules/headers into `tests/baseline/` and `tests/baseline/root/`. Verify the original archive package hashes from Ubuntu metadata.
2. Extract the five candidate packages into `artifacts/extracted/`. Extract packaged headers into the container's `/usr/src` only, preserving the container's `/lib` symlink. Do not extract header packages into a host filesystem.
3. Build `tests/rcu_probe` against the matching headers, separately for stock and candidate. Place `rcu_lifetime_probe.ko` in `tests/baseline/` or `tests/candidate/`. Sign the candidate probe with that candidate builder's key and certificate using `scripts/sign-file`. Decompress its native signed `dummy.ko.zst` into `tests/candidate/dummy.ko`.
4. Build `tests/build-test-initramfs.sh`, then `tests/build-probe-initramfs.sh baseline` and `candidate`. The four reuseport C tests come unchanged from the authenticated kernel's `tools/testing/selftests/net/` directory.
5. Build both disposable EFI images using `tests/build-efi-image.sh`. Required lab utilities include static GCC/libc, BusyBox, cpio, GRUB EFI tools, parted, dosfstools/mtools, QEMU and OVMF.
6. Run `tests/run-candidate-uefi.py` on the workstation. Set `OVMF_CODE` and `OVMF_VARS` if firmware paths differ. It attaches no NICs or host disks, creates fresh firmware variables and uses software TCG. The same runner with baseline filenames/image was used for stock147. Keep at least two virtual CPUs.

`verify-vm-results.py` records the original log-verification implementation. Its input/output schema is illustrated by `evidence/vm-tests.json`. Published result summaries are evidence from the recorded runs; this repository does not claim a newly executed clean end-to-end run of the convenience build wrapper or an automatic full VM orchestration command. The underlying native build commands and the supplied C/init harness were executed successfully.

A full production exploit or KASAN race reproducer was not run. The strongest focused substitute was the paired, real-destructor RCU lifetime probe plus genuine socket compatibility controls. Source comparisons checked all 59,603 original C/header/assembly/Rust files; only the two intended IPv4 files changed. ABI/module inventories and actual physical service checks complement that focused proof.
