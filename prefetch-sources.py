#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Cache exact checksum-pinned ancillary source packages used by native packaging."""
import hashlib,json,pathlib,urllib.request
base=pathlib.Path(__file__).resolve().parent
spec=json.loads((base/'ancillary-source-inputs.json').read_text());cache=base/spec['cache_directory'];cache.mkdir(parents=True,exist_ok=True)
for item in spec['packages']:
 p=cache/item['file']
 if not p.exists():
  with urllib.request.urlopen(item['url'],timeout=120) as r:p.write_bytes(r.read())
 assert hashlib.sha256(p.read_bytes()).hexdigest()==item['sha256'],p.name
 print('Verified '+p.name)
