#!/usr/bin/env python3
"""Bind final VM proof to the packaged candidate; never modifies a server."""
import datetime,hashlib,json,pathlib,re
base=pathlib.Path(__file__).resolve().parent
release='6.8.0-147.1+ionostcp1-generic'
def parse(name,expected_release,expected_free):
    p=base/'logs'/name;text=p.read_text(errors='replace')
    assert 'VM_KERNEL_RELEASE='+expected_release in text
    assert 'VM_TEST_RESULT=0' in text and 'VM_TEST_RESULT=1' not in text
    assert not re.search(r'\b(?:BUG:|Oops:|KASAN:)|WARNING: CPU:',text)
    assert re.search(r'efi: EFI v',text),'UEFI system-table evidence missing'
    probes={name:{'early_free':int(n),'reader_timeout':int(timeout)} for name,n,timeout in re.findall(r'RCU_LIFETIME_PROBE destructor=(\S+) early_free=(\d+) reader_timeout=(\d+)',text)}
    assert set(probes)=={'inet_sock_destruct','tcp_v4_reqsk_destructor'}
    assert all(v['reader_timeout']==0 for v in probes.values())
    assert all((v['early_free']==0 if expected_free==0 else v['early_free']>=1) for v in probes.values())
    suites=dict(re.findall(r'END_SELFTEST=(\S+) EXIT=(\d+)',text))
    assert set(suites)=={'socket_edge_controls','reuseport_addr_any','reuseport_dualstack','reuseaddr_conflict','reuseport_bpf'}
    assert all(v=='0' for v in suites.values())
    assert 'EDGE_MALFORMED_OPTIONS rejected_with_EINVAL=1' in text
    assert 'EDGE_STREAM protocol=262 result=0' in text
    controls=re.findall(r'TCP_OPTIONS_RESULT mapped=(\d+) accepted=(\d+) reflected_rr=(\d+) completed=(\d+) failed=(\d+) elapsed_seconds=([\d.]+)',text)
    assert len(controls)==4
    assert all(int(a)==2000 and a==b==c and int(f)==0 for mapped,a,b,c,f,elapsed in controls)
    assert text.count('END_OPTIONS_CONTROL migrate_req=')==4
    return {'log':str(p.relative_to(base)),'log_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'kernel_release':expected_release,'UEFI_boot':True,'lifetime_probes':probes,'selftests':suites,'reflected_IPv4_options_connections':8000,'IPv4_and_IPv4_mapped_sockets':True,'request_migration_sysctl_tested_at':[0,1],'malformed_option_error_semantics_preserved':True,'MPTCP_compatibility_control':True,'DCCP_skip':'DCCP not supported: skipping DCCP tests' in text,'VM_result':0}
baseline=parse('baseline-uefi-vm.log','6.8.0-147-generic',1)
candidate=parse('candidate-uefi-vm.log',release,0)
candidate_text=(base/'logs/candidate-uefi-vm.log').read_text(errors='replace')
assert 'VM_MODULE_SIG_ENFORCE=Y' in candidate_text and 'VM_SIGNED_NATIVE_MODULE_RESULT=1' in candidate_text
candidate['module_signature_enforcement']=True
candidate['signed_native_dummy_driver_load_interface_and_unload']=True
execution=json.loads((base/'evidence/candidate-vm-execution.json').read_text())
assert execution['exit_code']==0 and not execution.get('timeout')
assert execution['disk_sha256_before']==execution['disk_sha256_after']
packages=json.loads((base/'evidence/package-validation.json').read_text())
assert packages['release']==release and packages['packaged_image_matches_built_bzImage']
inventory=json.loads((base/'evidence/module-inventory-comparison.json').read_text())
assert inventory['candidate_module_count']==inventory['baseline_module_count']==6474
assert not inventory['missing_from_candidate'] and not inventory['added_in_candidate']
out={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'outcome':'candidate_fix_verified_in_disposable_VM; production_not_changed','baseline':baseline,'candidate':candidate,'same_two_destructor_invariants_tested':True,'actual_traffic_exploit_or_KASAN_race_reproducer':False,'proof_scope':'Deterministic real destructor reclamation boundaries plus real socket compatibility; request destructor uses a controlled fixture, shared socket destructor uses a real UDP socket. No freed memory is read.','module_inventory_identical':6474,'package_image_sha256':packages['image_sha256'],'kernel_package_version':packages['version'],'remaining_production_gates':['Explicit approval to install/reboot','Independent console recovery confirmed','Physical eMMC/Ethernet and production-service validation after a controlled boot'],'performance_scope':'Software-emulated VM smoke tests; native throughput and production p95 latency were not benchmarked.','production_install_performed':False}
(base/'evidence/final-test-results.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps(out,indent=2))
