import ctypes
import random
import struct
import sys
import unittest
import zlib
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from image_codec import pack_image, unpack_image

class CodecTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.native=ctypes.CDLL(str(Path('.build/lyke_stm32/image_test.dll').resolve()))
        cls.native.validate_image.argtypes=[ctypes.c_void_p,ctypes.c_size_t]
        cls.native.validate_image.restype=ctypes.c_int

    def validate(self,data):
        return self.native.validate_image(ctypes.create_string_buffer(bytes(data)),len(data))

    def app(self,size):
        data=bytearray((i*13+i//7)%256 for i in range(size))
        struct.pack_into('<II',data,0,0x200057f0,0x08002101)
        return bytes(data)

    def test_roundtrip_and_native_decoder(self):
        for size in [512,513,4095,4096,4097,65536]:
            app=self.app(size);package=pack_image(app)
            self.assertEqual(unpack_image(package),app)
            self.assertEqual(self.validate(package),1)

    def test_fuzz_recomputed_compressed_crc(self):
        rng=random.Random(20261123);p=pack_image(self.app(8192))
        for _ in range(250):
            data=bytearray(p);at=rng.randrange(24,len(data));data[at]^=1<<rng.randrange(8)
            struct.pack_into('<I',data,20,zlib.crc32(data[24:]))
            valid=self.validate(data)
            if valid: self.assertEqual(unpack_image(data),self.app(8192))

    def test_wrong_target_and_truncation(self):
        p=pack_image(self.app(4096))
        for cut in [0,8,23,24,25,len(p)-1]:self.assertFalse(self.validate(p[:cut]))
        bad=bytearray(self.app(4096));struct.pack_into('<I',bad,4,0x08000001)
        self.assertFalse(self.validate(pack_image(bad)))
        bad=bytearray(p);bad[0]=ord('X');self.assertFalse(self.validate(bad))

    def test_oversized_decompressed_block(self):
        app=self.app(8192);c=zlib.compressobj(9,zlib.DEFLATED,-13);coded=c.compress(app)+c.flush()
        body=struct.pack('<HH',len(coded),4096)+coded
        package=struct.pack('<8sIIII',b'LYKEG432',4096,zlib.crc32(app[:4096]),len(body),zlib.crc32(body))+body
        self.assertFalse(self.validate(package))

if __name__=='__main__':unittest.main()
