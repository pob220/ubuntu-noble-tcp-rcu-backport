#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Verify Ubuntu's signed Sources chain, exact source inputs and local patch."""
import argparse,hashlib,json,pathlib,re,subprocess,tarfile,tempfile
ap=argparse.ArgumentParser();ap.add_argument('downloads',type=pathlib.Path);ap.add_argument('--keyring',default='/usr/share/keyrings/ubuntu-archive-keyring.gpg');a=ap.parse_args()
repo=pathlib.Path(__file__).resolve().parent
pins=json.loads((repo/'source-inputs.json').read_text())
def digest(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
with tarfile.open(a.downloads/'source-authentication.tar.gz','r:gz') as tf:
 release=tf.extractfile('noble-updates.InRelease').read();sources=tf.extractfile('Sources').read()
with tempfile.TemporaryDirectory() as td:
 p=pathlib.Path(td)/'InRelease';p.write_bytes(release)
 subprocess.run(['gpgv','--keyring',str(pathlib.Path(a.keyring).resolve()),str(p)],check=True)
hash_sources=hashlib.sha256(sources).hexdigest()
assert re.search(r'^ '+hash_sources+r'\s+'+str(len(sources))+r'\s+main/source/Sources$',release.decode(),re.M)
entries=[s for s in sources.decode().split('\n\n') if re.search(r'^Package: linux$',s,re.M) and re.search(r'^Version: 6\.8\.0-147\.147$',s,re.M)]
assert len(entries)==1
checks={}
for line in entries[0].split('Checksums-Sha256:\n',1)[1].splitlines():
 f=line.split()
 if len(f)!=3:break
 checks[f[2]]=(f[0],int(f[1]))
assert set(checks)=={x['name'] for x in pins['files']}
for item in pins['files']:
 p=a.downloads/item['name'];sha,size=checks[p.name]
 assert (sha,size)==(item['sha256'],item['bytes'])
 assert p.stat().st_size==size and digest(p)==sha,p.name
assert digest(a.downloads/'kernel-and-packaging.patch')==pins['patch_sha256']
print('PASS: trusted Ubuntu signature -> Sources -> exact three source files; local patch hash matches manifest')
