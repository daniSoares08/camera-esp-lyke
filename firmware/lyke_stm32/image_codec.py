"""Independent raw DEFLATE blocks, bounded to 4096 output bytes each.

The loader uses Mark Adler's puff 2.3 decoder (zlib v1.3.1), with no allocator.
Full compressed and decompressed CRCs are verified before application erasure.
"""
import struct
import zlib

MAGIC = b'LYKEG432'
BLOCK = 4096
MAX_APP = 64 * 1024
MAX_STAGE = 48 * 1024


def encode_block(data):
    compressor = zlib.compressobj(9, zlib.DEFLATED, -13)
    return compressor.compress(data) + compressor.flush()


def decode_block(data, expected):
    if not 0 < expected <= BLOCK: raise ValueError('block length')
    decoder = zlib.decompressobj(-13)
    out = decoder.decompress(data, expected + 1)
    if len(out) != expected or not decoder.eof or decoder.unused_data or decoder.unconsumed_tail:
        raise ValueError('invalid DEFLATE block')
    return out


def pack_image(app):
    if not 512 <= len(app) <= MAX_APP: raise ValueError('application length')
    body = bytearray()
    for at in range(0, len(app), BLOCK):
        block = app[at:at+BLOCK]; coded = encode_block(block)
        body.extend(struct.pack('<HH', len(coded), len(block))); body.extend(coded)
    result = struct.pack('<8sIIII', MAGIC, len(app), zlib.crc32(app), len(body), zlib.crc32(body)) + body
    if len(result) > MAX_STAGE: raise ValueError('compressed image exceeds staging slot')
    if unpack_image(result) != app: raise ValueError('codec verification failed')
    return result


def unpack_image(package):
    if len(package) < 24: raise ValueError('short header')
    magic, size, crc, packed, packed_crc = struct.unpack_from('<8sIIII', package)
    if magic != MAGIC or not 512 <= size <= MAX_APP or len(package) != 24+packed or len(package)>MAX_STAGE:
        raise ValueError('header')
    body = package[24:]
    if zlib.crc32(body) != packed_crc: raise ValueError('packed CRC')
    out = bytearray(); at = 0
    while at < len(body):
        if at+4 > len(body): raise ValueError('block header')
        coded, plain = struct.unpack_from('<HH', body, at); at += 4
        if not 0 < plain <= BLOCK or at+coded>len(body): raise ValueError('block bounds')
        out.extend(decode_block(body[at:at+coded], plain)); at += coded
    if len(out) != size or zlib.crc32(out) != crc: raise ValueError('application CRC')
    return bytes(out)
