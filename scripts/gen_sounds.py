#!/usr/bin/env python3
"""
AnyClaw LVGL — 萌萌音效生成器
3 主题 × 8 音效 = 24 个 WAV
纯 Python 生成，无外部依赖。
"""

import wave
import struct
import math
import os
import random

SAMPLE_RATE = 44100

# ── 基础工具 ──────────────────────────────────────────────

def fade_in(buf, samples=200):
    for i in range(min(samples, len(buf))):
        buf[i] *= i / samples

def fade_out(buf, samples=400):
    n = len(buf)
    for i in range(min(samples, n)):
        buf[n - 1 - i] *= i / samples

def normalize(buf, peak=0.85):
    mx = max(abs(v) for v in buf) or 1
    scale = peak / mx
    return [v * scale for v in buf]

def mix(*buffers):
    length = max(len(b) for b in buffers)
    out = [0.0] * length
    for b in buffers:
        for i, v in enumerate(b):
            out[i] += v
    return normalize(out, 0.8)

def pad(buf, duration_ms):
    """Pad buf to total duration_ms."""
    target = int(SAMPLE_RATE * duration_ms / 1000)
    if len(buf) < target:
        buf.extend([0.0] * (target - len(buf)))
    return buf

def sine(freq, duration_ms, amp=0.5):
    n = int(SAMPLE_RATE * duration_ms / 1000)
    return [amp * math.sin(2 * math.pi * freq * i / SAMPLE_RATE) for i in range(n)]

def sine_sweep(f_start, f_end, duration_ms, amp=0.5):
    n = int(SAMPLE_RATE * duration_ms / 1000)
    buf = []
    for i in range(n):
        t = i / n
        freq = f_start + (f_end - f_start) * t
        buf.append(amp * math.sin(2 * math.pi * freq * i / SAMPLE_RATE))
    return buf

def triangle(freq, duration_ms, amp=0.4):
    """Soft triangle wave (warmer than sine for cute sounds)."""
    n = int(SAMPLE_RATE * duration_ms / 1000)
    buf = []
    period = SAMPLE_RATE / freq
    for i in range(n):
        phase = (i % period) / period
        val = 4 * abs(phase - 0.5) - 1
        buf.append(amp * val)
    return buf

def soft_click(freq, duration_ms=30, amp=0.3):
    """Tiny pop/click sound."""
    n = int(SAMPLE_RATE * duration_ms / 1000)
    buf = []
    for i in range(n):
        t = i / n
        envelope = math.exp(-t * 8)
        val = amp * envelope * math.sin(2 * math.pi * freq * i / SAMPLE_RATE)
        buf.append(val)
    return buf

def bubble_pop(freq, duration_ms=80, amp=0.4):
    """Cute bubble pop - rapid pitch drop."""
    n = int(SAMPLE_RATE * duration_ms / 1000)
    buf = []
    for i in range(n):
        t = i / n
        f = freq * (1 + 0.5 * (1 - t))  # pitch drops
        envelope = math.exp(-t * 6) * (1 + 0.3 * math.sin(2 * math.pi * 30 * t))
        buf.append(amp * envelope * math.sin(2 * math.pi * f * i / SAMPLE_RATE))
    return buf

def cute_chime(freqs, duration_ms, amp=0.35):
    """Sparkly chime with harmonics."""
    n = int(SAMPLE_RATE * duration_ms / 1000)
    buf = [0.0] * n
    for freq in freqs:
        for i in range(n):
            t = i / n
            env = math.exp(-t * 3) * (1 - t * 0.5)
            buf[i] += amp * env * math.sin(2 * math.pi * freq * i / SAMPLE_RATE)
    return buf

def bouncy_tone(freq, bounces, duration_ms, amp=0.35):
    """Bouncy baby sound - like 'boing boing'."""
    seg = duration_ms / bounces
    buf = []
    for b in range(bounces):
        n = int(SAMPLE_RATE * seg / 1000)
        for i in range(n):
            t = i / n
            f = freq * (1 + 0.15 * math.sin(2 * math.pi * 8 * t))
            env = math.exp(-t * 4) * (0.6 + 0.4 * (1 - b / bounces))
            buf.append(amp * env * math.sin(2 * math.pi * f * i / SAMPLE_RATE))
    return buf

def lfo_mod(freq, lfo_freq, lfo_depth, duration_ms, amp=0.4):
    """LFO-modulated tone (vibrato/tremolo)."""
    n = int(SAMPLE_RATE * duration_ms / 1000)
    buf = []
    for i in range(n):
        t = i / SAMPLE_RATE
        mod = 1 + lfo_depth * math.sin(2 * math.pi * lfo_freq * t)
        f = freq * mod
        env = 1.0 - (i / n) * 0.3
        buf.append(amp * env * math.sin(2 * math.pi * f * i / SAMPLE_RATE))
    return buf

def noise_burst(duration_ms=15, amp=0.08):
    """Tiny noise burst for texture."""
    n = int(SAMPLE_RATE * duration_ms / 1000)
    return [amp * (random.random() * 2 - 1) * math.exp(-i / n * 5) for i in range(n)]

# ── 音效定义 ──────────────────────────────────────────────

def generate_matcha_sounds():
    """Matcha 抹茶 — 清新精灵音，女声气质，薄荷感"""
    sounds = {}

    # 1. msg_incoming — 清脆三连叮 (女声 "叮~叮~叮")
    a = cute_chime([880, 1108, 1318], 200, 0.3)
    b = cute_chime([1318], 150, 0.25)
    b_delayed = [0.0] * int(SAMPLE_RATE * 0.12) + b
    sounds['msg_incoming'] = mix(a, b_delayed)

    # 2. msg_sent — 轻快上滑音 (女声 "咻~")
    sounds['msg_sent'] = sine_sweep(600, 1200, 120, 0.35)

    # 3. success — 清脆胜利铃 (女声 "叮~叮~叮~叮!")
    notes = [523, 659, 784, 1047]
    layers = []
    for i, f in enumerate(notes):
        ch = cute_chime([f], 250, 0.25)
        layers.append([0.0] * int(SAMPLE_RATE * i * 0.1) + ch)
    sounds['success'] = mix(*layers)

    # 4. error — 薄荷警示 (女声 "嘟~嘟")
    a = triangle(330, 100, 0.3)
    gap = [0.0] * int(SAMPLE_RATE * 0.06)
    b = triangle(280, 120, 0.25)
    sounds['error'] = mix(a, gap + b)

    # 5. click — 水滴声
    sounds['click'] = bubble_pop(1800, 40, 0.25)

    # 6. switch — 清风切换 (女声 "呼~")
    sounds['switch'] = sine_sweep(400, 800, 80, 0.3) + sine_sweep(800, 600, 80, 0.2)

    # 7. popup — 萌萌弹出 (女声 "啵!")
    sounds['popup'] = bubble_pop(1500, 60, 0.35) + cute_chime([2000], 100, 0.15)

    # 8. typing — 细碎精灵音
    buf = []
    for _ in range(6):
        buf.extend(soft_click(random.uniform(2000, 3500), 15, 0.12))
        buf.extend([0.0] * int(SAMPLE_RATE * random.uniform(0.03, 0.06)))
    sounds['typing'] = buf

    return sounds


def generate_peachy_sounds():
    """Peachy 桃气 — 温暖甜美音，男声气质，软萌"""
    sounds = {}

    # 1. msg_incoming — 温暖双音 (男声 "嗯~嘿")
    a = lfo_mod(440, 5, 0.02, 150, 0.3)
    b = [0.0] * int(SAMPLE_RATE * 0.08) + lfo_mod(554, 4, 0.02, 120, 0.25)
    sounds['msg_incoming'] = mix(a, b)

    # 2. msg_sent — 弹性发送 (男声 "噗~")
    sounds['msg_sent'] = bouncy_tone(500, 2, 100, 0.3)

    # 3. success — 温暖欢呼 (男声 "耶~!")
    notes = [392, 494, 587, 784]
    layers = []
    for i, f in enumerate(notes):
        t = lfo_mod(f, 3, 0.015, 200, 0.25)
        layers.append([0.0] * int(SAMPLE_RATE * i * 0.12) + t)
    sounds['success'] = mix(*layers)

    # 4. error — 温柔叹气 (男声 "唉~")
    a = lfo_mod(280, 6, 0.03, 200, 0.3)
    sounds['error'] = a

    # 5. click — 柔和弹跳
    sounds['click'] = bouncy_tone(1200, 1, 35, 0.2)

    # 6. switch — 温暖翻页 (男声 "哗~")
    sounds['switch'] = sine_sweep(300, 600, 70, 0.25) + noise_burst(20, 0.06)

    # 7. popup — 桃心弹出 (男声 "嘭~!")
    sounds['popup'] = bouncy_tone(800, 2, 120, 0.3) + cute_chime([1200], 80, 0.15)

    # 8. typing — 温暖按键
    buf = []
    for _ in range(6):
        buf.extend(bouncy_tone(random.uniform(600, 900), 1, 18, 0.1))
        buf.extend([0.0] * int(SAMPLE_RATE * random.uniform(0.04, 0.07)))
    sounds['typing'] = buf

    return sounds


def generate_mochi_sounds():
    """Mochi 糯 — 奶宝宝音，萌宝气质，软糯"""
    sounds = {}

    # 1. msg_incoming — 软糯呼唤 (宝宝 "糯~糯~")
    a = lfo_mod(330, 3, 0.04, 250, 0.3)
    b = [0.0] * int(SAMPLE_RATE * 0.1) + lfo_mod(392, 2.5, 0.04, 200, 0.25)
    c = [0.0] * int(SAMPLE_RATE * 0.22) + lfo_mod(440, 2, 0.03, 180, 0.2)
    sounds['msg_incoming'] = mix(a, b, c)

    # 2. msg_sent — 软软弹出 (宝宝 "噗~")
    sounds['msg_sent'] = bouncy_tone(350, 3, 150, 0.25)

    # 3. success — 萌宝欢呼 (宝宝 "哇~!") 
    notes = [262, 330, 392, 523]
    layers = []
    for i, f in enumerate(notes):
        t = lfo_mod(f, 2, 0.03, 280, 0.22)
        layers.append([0.0] * int(SAMPLE_RATE * i * 0.15) + t)
    sounds['success'] = mix(*layers)

    # 4. error — 软萌沮丧 (宝宝 "唔~")
    a = lfo_mod(220, 8, 0.05, 300, 0.28)
    sounds['error'] = a

    # 5. click — 糯米团轻触
    sounds['click'] = soft_click(800, 45, 0.18) + sine(600, 30, 0.1)

    # 6. switch — 糯米翻滚 (宝宝 "咕噜~")
    sounds['switch'] = sine_sweep(250, 500, 100, 0.22) + lfo_mod(400, 12, 0.06, 80, 0.1)

    # 7. popup — 糯米弹弹 (宝宝 "砰~!")
    sounds['popup'] = bouncy_tone(500, 4, 200, 0.25) + lfo_mod(700, 5, 0.03, 100, 0.12)

    # 8. typing — 糯米步
    buf = []
    for _ in range(5):
        buf.extend(soft_click(random.uniform(500, 750), 25, 0.08))
        buf.extend([0.0] * int(SAMPLE_RATE * random.uniform(0.05, 0.08)))
    sounds['typing'] = buf

    return sounds


# ── 写入 WAV ──────────────────────────────────────────────

def write_wav(path, samples, sample_rate=SAMPLE_RATE):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    samples = normalize(samples, 0.8)
    fade_in(samples, 100)
    fade_out(samples, 300)
    
    with wave.open(path, 'w') as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)  # 16-bit
        wf.setframerate(sample_rate)
        for s in samples:
            val = max(-1, min(1, s))
            wf.writeframes(struct.pack('<h', int(val * 32767)))
    
    dur_ms = len(samples) * 1000 / sample_rate
    print(f"  ✓ {os.path.basename(path):20s}  {dur_ms:6.0f}ms  {os.path.getsize(path)//1024:>3d}KB")


# ── 主流程 ────────────────────────────────────────────────

def main():
    base = os.path.join(os.path.dirname(__file__), 'assets', 'sounds')
    
    themes = {
        'matcha': generate_matcha_sounds,
        'peachy': generate_peachy_sounds,
        'mochi':  generate_mochi_sounds,
    }
    
    sound_names = [
        'msg_incoming', 'msg_sent', 'success', 'error',
        'click', 'switch', 'popup', 'typing'
    ]
    
    print("🎵 AnyClaw LVGL — 萌萌音效生成器")
    print("=" * 55)
    
    total = 0
    for theme, gen_fn in themes.items():
        print(f"\n🌿 {theme.upper()}")
        sounds = gen_fn()
        for name in sound_names:
            path = os.path.join(base, theme, f'{name}.wav')
            write_wav(path, sounds[name])
            total += 1
    
    print(f"\n{'=' * 55}")
    print(f"✅ 完成！生成 {total} 个音效文件")
    print(f"📁 输出目录: {os.path.abspath(base)}")


if __name__ == '__main__':
    random.seed(42)  # 可复现
    main()
