import os, sys, struct, argparse, hashlib, hmac

def readle(b):
    return int.from_bytes(b, 'little')

def readbe(b):
    return int.from_bytes(b, 'big')

def hextobytes(s):
    return bytes.fromhex(s)

# RSA keys, (retail, dev)
rsa_key_mod = (bytes.fromhex('BAF198A49F2E78F81DCBDCE57DB54FB77C6A158FA3F10DC19E1B95345CA6E714C93F44F04CD2D71F4E89EBEE2ED5BCFCA2E63F6B821D883C0098E5F67B7D217EDC77A1BBBD4C624D362BF2C6BE3D300E3ED3B8BE336047029E131D50C56EB67C195F968760915CAFDE38CA49F1D332DDA845D660FDCF4872E19CC5635488D5D7'), bytes.fromhex('E51CBFC7630B9DD166598D0F1AADB73A7DA8E94330B57017FF77A13E06F10856EBC63779DEC0CE485CBE81603D36FFBF9F97BFA43C98955C9CDA2BEB31BE72E3CAEBB2ECB0606A809446C6E0A47E52AD1A3F45906471B92020F7E7A3C9C85C9ECF5414F4B2921D61253631D81FC1FC009AC6EAE828F9CC73E738BF36F4A80FF9'))
rsa_key_priv = (None, bytes.fromhex('B57CC285E4F56CBC554116B62241FD64BDE9B16D620637972AECCEB35DB84D0CDD93949A5B538B9452B32DB4D888DAAA2677847D4AEAEB560381E74C55893163B4C5B95919A1CC46B571AFC176825BADB416B875BEF5A559CB3AE2C5784520F2C20674B151D94E904D2B7B85E481C30780A5941B239BAC7E5E8A16018B1EE569'))

def verify(consoleID, hwinfo, dev):
    h = hashlib.sha1()
    h.update(consoleID)
    hmac_key = h.digest()

    hm = hmac.new(key=hmac_key, digestmod=hashlib.sha1)
    hm.update(hwinfo[0x88:0xA4])
    hmac_digest = hm.digest()

    dec = pow(readbe(hwinfo[:0x80]), 0x10001, readbe(rsa_key_mod[dev])).to_bytes(0x80, 'big')
    
    if dec[-20:] == hmac_digest:
        print('Signature is valid')
    else:
        print('Signature is invalid')

def resign(consoleID, hwinfo, dev):
    if not dev:
        print('Not supported for retail')
        return
    
    h = hashlib.sha1()
    h.update(consoleID)
    hmac_key = h.digest()

    hm = hmac.new(key=hmac_key, digestmod=hashlib.sha1)
    hm.update(hwinfo[0x88:0xA4])
    hmac_digest = hm.digest()
    hmac_digest_padded = b'\x00\x01' + b'\xff' * 105 + b'\x00' + hmac_digest

    enc = pow(readbe(hmac_digest_padded), readbe(rsa_key_priv[dev]), readbe(rsa_key_mod[dev])).to_bytes(0x80, 'big')
    enc += b'\x00' * (0x80 - len(enc))
    
    out = 'HWINFO_S_resigned.dat'
    with open(out, 'wb') as f:
        f.write(enc)
        f.write(hwinfo[0x80:])
    print(f'Wrote to {out}')

parser = argparse.ArgumentParser()
parser.add_argument('--consoleid', required=True,
                    help='console id in hex')
parser.add_argument('--hwinfo', required=True,
                    help='path to the HWINFO_S.dat file')
parser.add_argument('mode', nargs=1,
                    help='verify or resign')
parser.add_argument('--dev', action='store_true',
                    help='use dev key')
args = parser.parse_args()
mode = args.mode[0]
if mode not in ['verify', 'resign']:
    raise Exception('Invalid mode')
if args.dev:
    dev = 1
else:
    dev = 0

consoleID = hextobytes(args.consoleid)
with open(args.hwinfo, 'rb') as f:
    hwinfo = f.read()

if mode == 'verify':
    verify(consoleID, hwinfo, dev)
elif mode == 'resign':
    resign(consoleID, hwinfo, dev)