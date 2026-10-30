#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-only
# Run as an unprivileged user in the disposable Ubuntu build container only.
set -euo pipefail
cd /work
[ "$(id -u)" != 0 ]
test ! -e source/linux-6.8.0
python3 verify-source.py downloads
mkdir -p source artifacts/packages logs
cd source
dpkg-source -x ../downloads/linux_6.8.0-147.147.dsc
cd linux-6.8.0
sh debian.master/reconstruct
patch --batch --fuzz=0 -p1 < /work/patches/0001-ipv4-defer-option-reclamation.patch
patch --batch --fuzz=0 -p1 < /work/patches/0002-local-package-namespace.patch
fakeroot debian/rules clean
cd /work
python3 prefetch-sources.py
cd source/linux-6.8.0
fakeroot debian/rules -j"${BUILD_JOBS:-12}" binary-headers binary-generic do_tools=false
# Select by exact package names: additional native build outputs are not installers.
cd /work
python3 - <<'PY'
import json,pathlib,shutil,subprocess
root=pathlib.Path('/work');manifest=json.loads((root/'packages.json').read_text())
expected={x['metadata']['Package'] for x in manifest['packages']};found=set()
for p in (root/'source').glob('*.deb'):
 name=subprocess.check_output(['dpkg-deb','-f',str(p),'Package'],text=True).strip()
 if name not in expected:continue
 assert name not in found;found.add(name)
 listing=subprocess.check_output(['dpkg-deb','--contents',str(p)],text=True)
 assert 'signing_key.pem' not in listing
 shutil.copyfile(p,root/'artifacts/packages'/p.name)
assert found==expected
PY
