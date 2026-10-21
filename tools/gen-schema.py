#!/usr/bin/env python3
import re
import sys

src, dst = sys.argv[1], sys.argv[2]
LINE = re.compile(r'^([a-zA-Z0-9_.]+)#([0-9a-f]+)\s*(.*?)\s*=\s*([a-zA-Z0-9_.<>]+);$')

decls = []
for raw in open(src, encoding='utf-8'):
    line = raw.strip()
    m = LINE.match(line)
    if not m:
        continue
    name, cid, body, _ = m.groups()
    fields = []
    for tok in body.split():
        if tok.startswith('{') or ':' not in tok:
            continue
        fname, spec = tok.split(':', 1)
        flag, bit = None, 0
        if '?' in spec:
            left, spec = spec.split('?', 1)
            flag, b = left.split('.')
            bit = int(b)
        fields.append((fname, spec, flag, bit))
    decls.append((int(cid, 16), name, fields))

decls.sort()
cids = [d[0] for d in decls]
assert len(cids) == len(set(cids)), 'duplicate constructor ids'

strings = {}


def s(text):
    if text not in strings:
        strings[text] = 's%d' % len(strings)
    return strings[text]


rows = []
for i, (cid, name, fields) in enumerate(decls):
    fl = []
    for fname, spec, flag, bit in fields:
        fl.append('{%s, %s, %s, %d}' % (s(fname), s(spec), s(flag) if flag else 'NULL', bit))
    rows.append((cid, s(name), fl))

with open(dst, 'w') as f:
    f.write('#include <stddef.h>\n#include "tg_tl.h"\n\n')
    for text, ident in sorted(strings.items(), key=lambda kv: int(kv[1][1:])):
        f.write('static const char %s[] = "%s";\n' % (ident, text))
    f.write('\n')
    for i, (cid, name, fl) in enumerate(rows):
        if fl:
            f.write('static const tg_field f%d[] = {%s};\n' % (i, ', '.join(fl)))
    f.write('\nconst tg_decl tg_decls[] = {\n')
    for i, (cid, name, fl) in enumerate(rows):
        f.write('    {0x%08xu, %s, %s, %d},\n' % (cid, name, 'f%d' % i if fl else 'NULL', len(fl)))
    f.write('};\n\nconst int tg_decl_count = %d;\n' % len(rows))
print('%d constructors, %d distinct strings -> %s' % (len(rows), len(strings), dst))
