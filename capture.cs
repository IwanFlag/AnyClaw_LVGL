using System;
using System.IO;
using System.Runtime.InteropServices;

public class WinHelper {
    [DllImport("user32.dll")] public static extern IntPtr GetWindowDC(IntPtr h);
    [DllImport("user32.dll")] public static extern int GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
    [DllImport("gdi32.dll")] public static extern IntPtr CreateCompatibleDC(IntPtr h);
    [DllImport("gdi32.dll")] public static extern IntPtr CreateCompatibleBitmap(IntPtr h, int w, int ht);
    [DllImport("gdi32.dll")] public static extern IntPtr SelectObject(IntPtr h, IntPtr o);
    [DllImport("gdi32.dll")] public static extern bool BitBlt(IntPtr d, int x, int y, int w, int h, IntPtr s, int sx, int sy, uint r);
    [DllImport("gdi32.dll")] public static extern bool DeleteObject(IntPtr o);
    [DllImport("gdi32.dll")] public static extern bool DeleteDC(IntPtr d);
    [DllImport("user32.dll")] public static extern int ReleaseDC(IntPtr w, IntPtr d);
    [DllImport("gdi32.dll")] public static extern int GetDIBits(IntPtr a, IntPtr b, int c, int e, IntPtr f, ref BITMAPINFO g, int u);

    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L,T,R,B; }
    [StructLayout(LayoutKind.Sequential)] public struct BITMAPINFOHEADER {
        public uint sz; public int w, h; public ushort p, bpp; public uint cm, imgSz; public int xp, yp; public uint cu, ci;
    }
    [StructLayout(LayoutKind.Sequential)] public struct BITMAPINFO { public BITMAPINFOHEADER h; }

    public static void Click(long hv, int x, int y) {
        IntPtr hWnd = (IntPtr)hv;
        SetForegroundWindow(hWnd);
        IntPtr lp = (IntPtr)((y << 16) | (x & 0xFFFF));
        PostMessage(hWnd, 0x0201, IntPtr.Zero, lp);
        PostMessage(hWnd, 0x0202, IntPtr.Zero, lp);
    }

    public static void Capture(long hv, string path) {
        IntPtr h = (IntPtr)hv;
        RECT r; GetWindowRect(h, out r);
        int w = r.R - r.L, ht = r.B - r.T;
        if (w <= 0 || ht <= 0) return;
        IntPtr dc = GetWindowDC(h), mdc = CreateCompatibleDC(dc);
        IntPtr bmp = CreateCompatibleBitmap(dc, w, ht);
        IntPtr old = SelectObject(mdc, bmp);
        BitBlt(mdc, 0, 0, w, ht, dc, 0, 0, 0x00CC0020);
        int stride = (w * 3 + 3) & ~3;
        int hdrSz = 54, fileSize = hdrSz + stride * ht;
        byte[] file = new byte[fileSize];
        file[0] = (byte)'B'; file[1] = (byte)'M';
        BitConverter.GetBytes(fileSize).CopyTo(file, 2);
        BitConverter.GetBytes(hdrSz).CopyTo(file, 10);
        BitConverter.GetBytes(40).CopyTo(file, 14);
        BitConverter.GetBytes(w).CopyTo(file, 18);
        BitConverter.GetBytes(ht).CopyTo(file, 22);
        BitConverter.GetBytes((ushort)1).CopyTo(file, 26);
        BitConverter.GetBytes((ushort)24).CopyTo(file, 28);
        BITMAPINFO bmi = new BITMAPINFO();
        bmi.h.sz = 40; bmi.h.w = w; bmi.h.h = ht; bmi.h.p = 1; bmi.h.bpp = 24;
        IntPtr bits = Marshal.AllocHGlobal(stride * ht);
        GetDIBits(mdc, bmp, 0, ht, bits, ref bmi, 0);
        for (int y = 0; y < ht; y++) {
            int src = (ht - 1 - y) * stride, dst = hdrSz + y * stride;
            Marshal.Copy(bits + src, file, dst, stride);
        }
        Marshal.FreeHGlobal(bits);
        File.WriteAllBytes(path, file);
        SelectObject(mdc, old); DeleteObject(bmp); DeleteDC(mdc); ReleaseDC(h, dc);
    }
}
