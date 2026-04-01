import ctypes
user32 = ctypes.windll.user32
EnumWindows = user32.EnumWindows
EnumWindowsProc = ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_int, ctypes.POINTER(ctypes.c_int))
GetWindowTextLength = user32.GetWindowTextLengthW
GetWindowText = user32.GetWindowTextW

def find_windows():
    results = []
    def callback(hwnd, _):
        length = GetWindowTextLength(hwnd)
        if length > 0:
            buf = ctypes.create_unicode_buffer(length + 1)
            GetWindowText(hwnd, buf, length + 1)
            title = buf.value
            results.append((hwnd, title))
        return True
    EnumWindows(EnumWindowsProc(callback), 0)
    return results

for hwnd, title in find_windows():
    t = title.lower()
    if 'any' in t or 'lvgl' in t or 'demo' in t:
        print('HWND={} Title="{}"'.format(hwnd, title))
