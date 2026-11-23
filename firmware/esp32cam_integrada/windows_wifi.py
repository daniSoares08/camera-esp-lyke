"""Native Windows Wi-Fi radio control; no credentials are read from Windows."""
import ctypes as c
from ctypes import wintypes as w

class GUID(c.Structure):
    _fields_=[('a',w.DWORD),('b',w.WORD),('d',w.WORD),('e',c.c_ubyte*8)]

class Info(c.Structure):
    _fields_=[('guid',GUID),('description',w.WCHAR*256),('state',c.c_int)]

class Radio(c.Structure):
    _fields_=[('phy',w.DWORD),('software',c.c_int),('hardware',c.c_int)]

def set_radio(enabled):
    api=c.WinDLL('wlanapi');handle=w.HANDLE();version=w.DWORD();ptr=c.c_void_p()
    rc=api.WlanOpenHandle(2,None,c.byref(version),c.byref(handle))
    if rc: raise OSError(rc,'WlanOpenHandle')
    try:
        rc=api.WlanEnumInterfaces(handle,None,c.byref(ptr))
        if rc: raise OSError(rc,'WlanEnumInterfaces')
        count=c.cast(ptr,c.POINTER(w.DWORD))[0]
        for i in range(count):
            info=Info.from_address(ptr.value+8+i*c.sizeof(Info))
            state=Radio(0,1 if enabled else 2,1)
            rc=api.WlanSetInterface(handle,c.byref(info.guid),4,c.sizeof(state),c.byref(state),None)
            if rc: raise OSError(rc,'WlanSetInterface radio state')
    finally:
        if ptr: api.WlanFreeMemory(ptr)
        api.WlanCloseHandle(handle,None)

if __name__=='__main__':
    import argparse
    p=argparse.ArgumentParser();p.add_argument('state',choices=['on','off']);args=p.parse_args()
    set_radio(args.state=='on');print('Wi-Fi software radio '+args.state)
