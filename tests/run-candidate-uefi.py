#!/usr/bin/env python3
"""Host-side disposable VM runner. No NIC, no host disk, no host modules."""
import hashlib,json,pathlib,shutil,subprocess,time
base=pathlib.Path(__file__).resolve().parents[1]
import os
firmware=pathlib.Path(os.environ.get('OVMF_CODE','/usr/share/OVMF/OVMF_CODE_4M.fd'))
template=pathlib.Path(os.environ.get('OVMF_VARS','/usr/share/OVMF/OVMF_VARS_4M.fd'))
variables=base/'tests/candidate-OVMF_VARS.fd';shutil.copyfile(template,variables)
disk=base/'tests/candidate-efi.img';log=base/'logs/candidate-uefi-vm.log'
argv=['qemu-system-x86_64','-machine','q35','-accel','tcg,thread=multi','-cpu','max','-smp','2','-m','1536','-display','none','-monitor','none','-serial','file:'+str(log),'-nic','none','-no-reboot','-drive','if=pflash,format=raw,unit=0,readonly=on,file='+str(firmware),'-drive','if=pflash,format=raw,unit=1,file='+str(variables),'-drive','if=virtio,format=raw,file='+str(disk)]
record={'argv':argv,'firmware_sha256':hashlib.sha256(firmware.read_bytes()).hexdigest(),'fresh_variables_template_sha256':hashlib.sha256(template.read_bytes()).hexdigest(),'disk_sha256_before':hashlib.sha256(disk.read_bytes()).hexdigest(),'physical_NICs':0,'host_disks_attached':0,'accelerator':'TCG software emulation; native throughput not assessed'}
start=time.monotonic()
try:
    r=subprocess.run(argv,capture_output=True,text=True,timeout=300)
    record.update({'exit_code':r.returncode,'stderr':r.stderr,'wall_seconds':round(time.monotonic()-start,3)})
except subprocess.TimeoutExpired:
    record.update({'timeout':True,'wall_seconds':round(time.monotonic()-start,3)})
record['disk_sha256_after']=hashlib.sha256(disk.read_bytes()).hexdigest()
(base/'evidence/candidate-vm-execution.json').write_text(json.dumps(record,indent=2)+'\n')
print(json.dumps(record,indent=2))
assert record.get('exit_code')==0 and not record.get('timeout'),record
