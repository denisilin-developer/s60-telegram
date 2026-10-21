#!/usr/bin/env python3
import base64
import sys

pem_path, prime_hex, out_path = sys.argv[1], sys.argv[2], sys.argv[3]


def der_ints(der):
    pos = 0

    def tag_len():
        nonlocal pos
        tag = der[pos]
        first = der[pos + 1]
        pos += 2
        if first & 0x80:
            count = first & 0x7F
            length = int.from_bytes(der[pos:pos + count], 'big')
            pos += count
        else:
            length = first
        return tag, length

    tag, _ = tag_len()
    assert tag == 0x30
    out = []
    for _ in range(2):
        tag, length = tag_len()
        assert tag == 0x02
        out.append(int.from_bytes(der[pos:pos + length], 'big'))
        pos += length
    return out


def minimal(n):
    return n.to_bytes((n.bit_length() + 7) // 8, 'big')


def c_bytes(b):
    return ', '.join('0x%02x' % x for x in b)


keys = []
for chunk in open(pem_path).read().split('-----BEGIN')[1:]:
    if 'RSA PUBLIC KEY' not in chunk:
        continue
    body = chunk.split('-----', 2)[1]
    body = body.split('-----END')[0]
    der = base64.b64decode(''.join(body.split()))
    keys.append(der_ints(der))

prime = bytes.fromhex(prime_hex)
assert len(prime) == 256

with open(out_path, 'w') as f:
    f.write('#include "tg_crypto.h"\n\n')
    for i, (n, e) in enumerate(keys):
        f.write('static const unsigned char key%d_n[] = {%s};\n' % (i, c_bytes(minimal(n))))
        f.write('static const unsigned char key%d_e[] = {%s};\n' % (i, c_bytes(minimal(e))))
    f.write('\nconst tg_rsa_key tg_rsa_keys[] = {\n')
    for i, (n, e) in enumerate(keys):
        f.write('    {key%d_n, %d, key%d_e, %d},\n' % (i, len(minimal(n)), i, len(minimal(e))))
    f.write('};\n\nconst int tg_rsa_key_count = %d;\n\n' % len(keys))
    f.write('const unsigned char tg_dh_prime[256] = {%s};\n' % c_bytes(prime))
print('%d RSA keys + DH prime -> %s' % (len(keys), out_path))
