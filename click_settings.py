import ctypes, time

user32 = ctypes.windll.user32
hwnd = user32.FindWindowW(None, 'AnyClaw LVGL v2.0 - Desktop Manager')
if not hwnd:
    print('Window not found')
    exit()

rect = ctypes.create_string_buffer(16)
user32.GetWindowRect(hwnd, rect)
left = int.from_bytes(rect[0:4], 'little')
top = int.from_bytes(rect[4:8], 'little')
right = int.from_bytes(rect[8:12], 'little')
bottom = int.from_bytes(rect[12:16], 'little')
print('Window:', left, top, right-left, bottom-top)

content_x = left + 4
content_y = top + 39
scale_x = (right - left - 8) / 1450.0
scale_y = (bottom - top - 43) / 800.0

# Settings button at approximately (1185, 340)
btn_x = int(content_x + 1185 * scale_x)
btn_y = int(content_y + 340 * scale_y)
print(f'Clicking settings at ({btn_x}, {btn_y})')

user32.SetCursorPos(btn_x, btn_y)
time.sleep(0.3)
user32.mouse_event(2, 0, 0, 0, 0)  # MOUSEEVENTF_LEFTDOWN
time.sleep(0.1)
user32.mouse_event(4, 0, 0, 0, 0)  # MOUSEEVENTF_LEFTUP
time.sleep(1)
print('Click done')
