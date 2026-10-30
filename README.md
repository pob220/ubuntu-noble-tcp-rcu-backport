# Ubuntu Noble IPv4 RCU lifetime backport

A temporary Ubuntu 24.04 amd64 backport of **Eric Dumazet's already accepted upstream fix** for CVE-2026-98432, built from Ubuntu `linux 6.8.0-147.147`. This is a local community build, with an unsigned EFI image; it is not an official Canonical security release.

- Debian version: `6.8.0-147.147+ionostcp1`
- Kernel release: `6.8.0-147.1+ionostcp1-generic`
- Upstream commit: [`e3f33b0a1d89`](https://github.com/torvalds/linux/commit/e3f33b0a1d89d738e2b913dab519226eaf76e15b)
- Vendor status: [Canonical CVE tracker](https://ubuntu.com/security/CVE-2026-98432). On 30 October 2026, Noble's generic package remained `6.8.0-147.147`; Canonical described `6.8.0-149.149` as work in progress. Recheck before using this interim build.
- [Packages and corresponding source inputs](https://github.com/pob220/ubuntu-noble-tcp-rcu-backport/releases/tag/v6.8.0-147.147-ionostcp1)

The security change consists of two reclamation operations: `inet_sock_destruct()` and `tcp_v4_reqsk_destructor()` use `kfree_rcu()` so existing RCU readers finish before IPv4 option storage is reclaimed. Both boundaries matter because ownership transfers to child sockets and request migration shares option storage. The separately described IPv6 CALIPSO issue is outside this patch.

`patches/0001` contains the upstream repair with attribution. `patches/0002` gives the local packages distinct names without changing numeric C ABI 147 or generic configuration. It preserves vendor patches. Stock147 < local147.1 < official149 in the tested native GRUB sorter; the Debian version also sorts below future vendor revisions. Version ordering alone does not establish security coverage.

## Evidence

The candidate passed disposable two-vCPU QEMU UEFI/GRUB boots, followed by a physical UEFI boot on an eMMC/r8169 Ubuntu host. Root filesystem and native driver checks passed; no kernel panic, oops, BUG or CPU warning was detected. The stock recovery image remained unchanged. Public evidence contains kernel validation only.

The VM-only regression probe observed early frees at **both real destructor boundaries** on stock147 and zero early frees on the repaired candidate, with zero reader timeouts. It never dereferences freed memory. The socket destructor uses a real UDP socket; the request destructor uses a controlled fixture and resolves its real static function through the kprobe API. This is a deterministic reclamation-boundary test, not a complete network exploit or KASAN race reproduction.

Five socket selftests passed, along with 8,000 reflected IPv4-option connections per tested kernel covering IPv4/mapped sockets and migration sysctl settings 0/1. Candidate module-signature enforcement and native signed module load/unload passed. All 6,474 module paths and all 30,390 exported symbol CRCs match stock147. Only `CONFIG_VERSION_SIGNATURE` differs from the original generic configuration. DCCP was skipped because its optional module was absent from the minimal VM initramfs. Native throughput and long-duration stability were not benchmarked.

See `evidence/`, the release’s `config-6.8.0-147.1+ionostcp1-generic`, and [build and VM instructions](BUILD.md). Tests under `tests/rcu_probe` are for disposable two-vCPU VMs only. They are not part of the published kernel installer and must not be loaded on production hosts.

## Download and recovery

Download the five `.deb` files, `SHA256SUMS`, source archives, local patch and source-authentication archive from the release. Verify all files with `sha256sum -c SHA256SUMS` in a dedicated download directory. The manifest records the exact package dependencies and hashes. No private signing key is distributed.

The release's original tarball, Ubuntu diff, `.dsc`, local patch and build instructions provide the corresponding source and packaging changes. `verify-source.py` independently checks the archived Ubuntu signature and Sources checksums using the trusted Ubuntu archive keyring. Its recorded historical metadata establishes provenance; it is not a freshness check for today's available security updates.

Before installation, confirm hardware compatibility, free boot space, working console access and an intact old kernel/initramfs/modules set. Use an APT simulation with the exact five paths; retain the old boot as default during a one-time trial. These unsigned EFI-image packages do not provide a Canonical Secure Boot signature. Do not disable Secure Boot just to use them; prefer a released vendor fix or an independently administered signing/enrolment process where applicable.

The local release has distinct boot/module/header paths. Roll back through GRUB's advanced menu to the retained stock image if necessary. The stock147 image is affected by this vulnerability, so recovery also requires the earlier tactical mitigation until a repaired image is active. Install a verified official fixed kernel when available, test it and then retire this interim build.

## Attribution and licensing

The kernel fix is Eric Dumazet's upstream work. Ubuntu packaging and source retain their upstream and Canonical notices. This repository contributes the local backport packaging and build/test evidence; it makes no claim to have discovered the vulnerability or invented its repair. The review PR is in this community repository, not a new upstream Linux submission.

See `COPYING`, `LICENSES/preferred/GPL-2.0` and the Linux syscall exception. New helper scripts and test code are under GPL-2.0-only except where an existing file states its applicable upstream licence.
