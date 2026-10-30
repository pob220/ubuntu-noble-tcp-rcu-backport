# Ubuntu Noble IPv4 RCU lifetime backport

Community backport, build records and downloadable packages for Ubuntu 24.04 amd64. The security repair is Eric Dumazet’s already accepted upstream commit e3f33b0a1d89, addressing CVE-2026-98432. This is an interim local build, not a Canonical release.

The narrowly scoped backport and validation material are on the [`backport/cve-2026-98432-noble`](https://github.com/pob220/ubuntu-noble-tcp-rcu-backport/tree/backport/cve-2026-98432-noble) review branch. See [pull requests](https://github.com/pob220/ubuntu-noble-tcp-rcu-backport/pulls) and [release downloads](https://github.com/pob220/ubuntu-noble-tcp-rcu-backport/releases).

Prefer a verified official fixed kernel when available. Check [Canonical’s advisory](https://ubuntu.com/security/CVE-2026-98432) before using an interim build.
