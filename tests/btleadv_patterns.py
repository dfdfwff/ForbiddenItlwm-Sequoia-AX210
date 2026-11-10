#!/usr/bin/env python3
"""Check the patch table against this Mac's real bluetoothd.

This shows each pattern matches exactly once and changes only what it should. It
doesn't show that the Bluetooth controller accepts extended advertising.
"""
import re
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=(root/'BTLEAdvFixup/BTLEAdvPatches.hpp').read_text()
def table(name):
    body=re.search(r'%s\[\] = \{(.*?)\};'%name,source,re.S).group(1)
    return bytes(int(x,16) for x in re.findall(r'0x[0-9a-f]{2}',body))
# Read the x86_64 slice directly. lipo writing from a mapped copy of this file failed
# once with "Bad address" on a cold page cache, while a plain read was fine.
raw=Path('/usr/sbin/bluetoothd').read_bytes()
assert raw[:4]==bytes.fromhex('cafebabe')
binary=None
for i in range(int.from_bytes(raw[4:8],'big')):
    entry=raw[8+20*i:28+20*i]
    if int.from_bytes(entry[:4],'big')==0x1000007:
        offset=int.from_bytes(entry[8:12],'big');binary=raw[offset:offset+int.from_bytes(entry[12:16],'big')]
assert binary,'no x86_64 slice'
# With the kext loaded, a read can return pages it already patched, from the page cache.
# Put those back so the rest of this check always sees the original bytes.
for name in ('advertising','scanning','randomAddress','extendedConnection'):
    find,replace=table(name+'Find'),table(name+'Replace')
    if binary.count(find)==0 and binary.count(replace)==1:
        binary=binary.replace(replace,find)
        print('%s: this read returned the patched page, checked against the original bytes'%name)
checks=[('advertising',bytes.fromhex('ff9118090000')),('scanning',bytes.fromhex('ff91a8090000'))]
for name,query in checks:
    find,replace=table(name+'Find'),table(name+'Replace')
    assert len(find)==len(replace)
    if binary.count(find)!=1:
        raise SystemExit('%s pattern matches %d times: not the expected bluetoothd build'%(name,binary.count(find)))
    offset=binary.index(find)
    assert offset//4096==(offset+len(find)-1)//4096,'pattern crosses a page, Lilu may miss it'
    assert binary.count(replace)==0
    changed=[i for i in range(len(find)) if find[i]!=replace[i]]
    # Only the je (2 to 7) and the capability query (19 to 24) change.
    assert set(changed)<=set(range(2,8))|set(range(19,25)),changed
    assert find[2:4]==bytes.fromhex('0f84') and replace[2:8]==b'\x90'*6
    assert find[19:25]==query and replace[19:25]==bytes.fromhex('b80100000090')
find,replace=table('randomAddressFind'),table('randomAddressReplace')
if binary.count(find)!=1:
    raise SystemExit('randomAddress pattern matches %d times'%binary.count(find))
offset=binary.index(find)
assert offset//4096==(offset+len(find)-1)//4096,'pattern crosses a page'
assert len(find)==len(replace) and find[:9]==replace[:9] and binary.count(replace)==0
assert find[9:11]==bytes.fromhex('31ff') and find[11]==0xe8 and replace[9:]==b'\x90'*7
find,replace=table('extendedConnectionFind'),table('extendedConnectionReplace')
if binary.count(find)!=1:
    raise SystemExit('extendedConnection pattern matches %d times'%binary.count(find))
offset=binary.index(find)
assert offset//4096==(offset+len(find)-1)//4096,'pattern crosses a page'
assert len(find)==len(replace) and find[7:]==replace[7:] and binary.count(replace)==0
assert find[:3]==bytes.fromhex('0fb6bd') and replace[:7]==bytes.fromhex('bf010000009090')
print('BTLEAdvFixup patterns: passed against this bluetoothd (no controller test)')
