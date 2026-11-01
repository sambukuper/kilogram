import io
import re

BS = chr(92)
pat = re.compile(r'([A-Za-z0-9_\\/.\-]+\.(?:cpp|h|mm))\((\d+)(?:,\d+)?\):\s+error\s+(C\d+):\s+(.{1,100})')
seen = set()
out = []
for line in io.open('.winlog.txt', encoding='utf-8', errors='replace'):
    if ': error C' not in line:
        continue
    m = pat.search(line)
    if m:
        key = (m.group(1).replace(BS, '/'), m.group(4)[:70])
        if key in seen:
            continue
        seen.add(key)
        out.append('%s:%s: %s: %s' % (
            m.group(1).replace(BS, '/'), m.group(2), m.group(3), m.group(4)))
print(len(out), 'unique')
for o in out:
    print(o)
