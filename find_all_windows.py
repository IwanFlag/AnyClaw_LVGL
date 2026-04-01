import ctypes
user32 = ctypes.windll.user32
EnumWindows = user32.EnumWindows
EnumWindowsProc = ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_int, ctypes.POINTER(ctypes.c_int))
GetWindowTextLength = user32.GetWindowTextLength
GetWindowText = user32.GetWindowText
IsWindowVisible = user32.IsWindowVisible
GetWindowThreadProcessId = user32.GetWindowThreadProcessId

def get_all_windows():
    hwnds = []
    def callback(hwnd, _):
        length = GetWindowTextLength(hwnd)
        if length > 0 and IsWindowVisible(hwnd):
            buf = ctypes.create_unicode_buffer(length + 1)
            GetWindowText(hwnd, buf, length + 1)
            title = buf.value
            pid = ctypes.c_ulong(0)
            GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
            hwnds.append((hwnd, title, pid.value))
        return True
    EnumWindows(EnumWindowsProc(callback), 0)
    return hwnds

windows = get_all_windows()
for hwnd, title, pid in windows:
    print(f'HWND={hwnd} PID={pid} Title="{title}"')
