#!/usr/bin/env python3
"""
AnyClaw LVGL — 萌萌音效生成器 v2
3 主题 × 8 音效 = 24 个 WAV
带泛音、共振峰、混响、颤音，模拟真实人声质感。
"""

import wave
import struct
import math
import os
import random

SAMPLE_RATE = 44100
PI2 = 2 * math.pi

# ── 基础工具 ──────────────────────────────────────────────

def fade_in(buf, samples=300):
    for i in range(min(samples, len(buf))):
        buf[i] *= (i / samples) ** 0.5  # ease-out curve

def fade_out(buf, samples=800):
    n = len(buf)
    for i in range(min(samples, n)):
        buf[n - 1 - i] *= (i / samples) ** 0.7

def normalize(buf, peak=0.82):
    mx = max(abs(v) for v in buf) or 1
    return [v * peak / mx for v in buf]

def mix(*buffers, weights=None):
    if weights is None:
        weights = [1.0] * len(buffers)
    length = max(len(b) for b in buffers)
    out = [0.0] * length
    for b, w in zip(buffers, weights):
        for i, v in enumerate(b):
            out[i] += v * w
    return normalize(out, 0.82)

def envelope_adsr(n, attack=0.05, decay=0.15, sustain=0.7, release=0.3):
    """ADSR envelope for musical sounds."""
    buf = []
    a_end = int(n * attack)
    d_end = int(n * (attack + decay))
    r_start = int(n * (1 - release))
    for i in range(n):
        if i < a_end:
            buf.append(i / a_end)
        elif i < d_end:
            t = (i - a_end) / (d_end - a_end)
            buf.append(1.0 - t * (1 - sustain))
        elif i < r_start:
            buf.append(sustain)
        else:
            t = (i - r_start) / (n - r_start)
            buf.append(sustain * (1 - t))
    return buf

def apply_env(buf, env):
    return [v * e for v, e in zip(buf, env)]

# ── 波形引擎 ──────────────────────────────────────────────

def sine(freq, duration_ms, amp=0.5):
    n = int(SAMPLE_RATE * duration_ms / 1000)
    return [amp * math.sin(PI2 * freq * i / SAMPLE_RATE) for i in range(n)]

def harmonics(freq, duration_ms, amps=None, amp=0.4):
    """Rich tone with harmonics (organ-like warmth)."""
    if amps is None:
        amps = [1.0, 0.5, 0.3, 0.15, 0.08, 0.04]  # natural harmonics
    n = int(SAMPLE_RATE * duration_ms / 1000)
    buf = [0.0] * n
    for h, a in enumerate(amps, 1):
        f = freq * h
        if f > SAMPLE_RATE / 2:
            break
        for i in range(n):
            buf[i] += a * math.sin(PI2 * f * i / SAMPLE_RATE)
    return normalize([v * amp for v in buf], amp)

def formant(freq, duration_ms, formants=None, amp=0.4):
    """Vocal formant synthesis - sounds like a voice saying a vowel.
    formants: list of (formant_freq, bandwidth, amplitude)"""
    if formants is None:
        # Default "ah" vowel
        formants = [
            (800, 80, 1.0),
            (1200, 120, 0.5),
            (2500, 150, 0.2),
        ]
    n = int(SAMPLE_RATE * duration_ms / 1000)
    buf = [0.0] * n
    for ff, bw, fa in formants:
        # Each formant is a resonant filter on the fundamental
        for i in range(n):
            t = i / SAMPLE_RATE
            # Source: glottal pulse (sawtooth-ish with harmonics)
            src = 0.0
            for h in range(1, 8):
                src += math.sin(PI2 * freq * h * t) / h
            # Resonance: bandpass at formant freq
            resonance = math.exp(-bw * 0.001 * abs(math.sin(PI2 * ff * t)))
            buf[i] += fa * src * resonance * 0.15
    return normalize([v * amp for v in buf], amp)

def vibrato(freq, rate=5.5, depth=0.02, duration_ms=200, amp=0.4):
    """Tone with vibrato (wobble)."""
    n = int(SAMPLE_RATE * duration_ms / 1000)
    buf = []
    for i in range(n):
        t = i / SAMPLE_RATE
        mod = 1 + depth * math.sin(PI2 * rate * t)
        buf.append(amp * math.sin(PI2 * freq * mod * t))
    return buf

def sweep(f_start, f_end, duration_ms, amp=0.4, curve=1.0):
    """Frequency sweep with exponential curve."""
    n = int(SAMPLE_RATE * duration_ms / 1000)
    buf = []
    for i in range(n):
        t = (i / n) ** curve
        freq = f_start * (f_end / f_start) ** t
        buf.append(amp * math.sin(PI2 * freq * i / SAMPLE_RATE))
    return buf

def drop(freq, duration_ms, drop_semitones=12, amp=0.4):
    """Pitch drop (like a vocal 'uh-oh')."""
    n = int(SAMPLE_RATE * duration_ms / 1000)
    buf = []
    ratio = 2 ** (-drop_semitones / 12)
    for i in range(n):
        t = i / n
        f = freq * (1 - t + t * ratio)
        env = math.exp(-t * 3)
        buf.append(amp * env * math.sin(PI2 * f * i / SAMPLE_RATE))
    return buf

def bounce(freq, bounces, duration_ms, amp=0.35):
    """Bouncy boing sound with decreasing height."""
    total_n = int(SAMPLE_RATE * duration_ms / 1000)
    seg_n = total_n // bounces
    buf = []
    for b in range(bounces):
        decay = 1.0 - (b / bounces) * 0.6
        for i in range(seg_n):
            t = i / seg_n
            f = freq * (1 + 0.2 * math.sin(PI2 * 15 * t))  # wobble
            env = math.exp(-t * 5) * decay
            buf.append(amp * env * math.sin(PI2 * f * i / SAMPLE_RATE))
    # Pad remaining
    while len(buf) < total_n:
        buf.append(0.0)
    return buf[:total_n]

def sparkle(freqs, duration_ms, amp=0.3):
    """Sparkly chime with harmonics."""
    n = int(SAMPLE_RATE * duration_ms / 1000)
    buf = [0.0] * n
    for freq in freqs:
        for i in range(n):
            t = i / n
            env = math.exp(-t * 2.5) * (1 - t * 0.3)
            # Add shimmer harmonics
            val = math.sin(PI2 * freq * i / SAMPLE_RATE)
            val += 0.3 * math.sin(PI2 * freq * 2 * i / SAMPLE_RATE)
            val += 0.1 * math.sin(PI2 * freq * 3 * i / SAMPLE_RATE)
            buf[i] += amp * env * val / 1.4
    return buf

def noise(duration_ms, amp=0.05, filter_freq=None):
    """Filtered noise burst."""
    n = int(SAMPLE_RATE * duration_ms / 1000)
    buf = []
    prev = 0.0
    for i in range(n):
        t = i / n
        val = (random.random() * 2 - 1) * math.exp(-t * 6)
        if filter_freq:
            # Simple low-pass
            rc = 1.0 / (PI2 * filter_freq)
            dt = 1.0 / SAMPLE_RATE
            alpha = dt / (rc + dt)
            val = alpha * val + (1 - alpha) * prev
            prev = val
        buf.append(amp * val)
    return buf

def delay_echo(buf, delay_ms=60, feedback=0.3, mix_level=0.25):
    """Add echo/delay effect."""
    delay_n = int(SAMPLE_RATE * delay_ms / 1000)
    out = list(buf)
    for i in range(delay_n, len(out)):
        out[i] += out[i - delay_n] * feedback
    # Mix original + echo
    return [o * (1 - mix_level) + b * mix_level for o, b in zip(out, buf)]

def reverb(buf, decay=0.4, taps=None):
    """Simple reverb using multiple delay taps."""
    if taps is None:
        taps = [23, 37, 53, 71]  # prime number delays in ms
    out = list(buf)
    for tap_ms in taps:
        tap_n = int(SAMPLE_RATE * tap_ms / 1000)
        gain = decay / len(taps)
        for i in range(tap_n, len(out)):
            out[i] += out[i - tap_n] * gain
    return normalize(out, 0.82)

# ── 三主题音效 ────────────────────────────────────────────

# ─── Matcha 抹茶 — 清新女声精灵感 ───
# 高音域、清脆、薄荷绿感、女声气质
# 音色：明亮泛音 + 金属感铃声 + 快速响应

def matcha_incoming():
    """收到消息 — 清脆女声三连叮 'di~di~di!' """
    # Bell-like harmonics, bright and clear
    a = sparkle([1047, 1319, 1568], 180, 0.3)  # C6-E6-G6 chord
    b = sparkle([1568], 200, 0.25)
    b = [0.0] * int(SAMPLE_RATE * 0.1) + b
    # Add shimmer
    shimmer = sine(2093, 150, 0.06)  # C7 overtone
    shimmer = [0.0] * int(SAMPLE_RATE * 0.05) + shimmer
    result = mix(a, b, shimmer)
    return reverb(result, 0.3, [19, 31, 47])

def matcha_sent():
    """发送消息 — 轻快上滑 'pii~' """
    up = sweep(700, 1400, 90, 0.35, curve=0.6)
    # Add sparkle on top
    top = sparkle([1400, 1760], 60, 0.12)
    top = [0.0] * int(SAMPLE_RATE * 0.04) + top
    return mix(up, top)

def matcha_success():
    """成功 — 胜利铃 'ding~ding~ding~ding!' """
    notes = [523, 659, 784, 1047]  # C5-E5-G5-C6
    layers = []
    for i, f in enumerate(notes):
        ch = sparkle([f], 300, 0.22)
        # Add octave shimmer
        ch2 = sparkle([f * 2], 200, 0.08)
        ch = mix(ch, ch2)
        layers.append([0.0] * int(SAMPLE_RATE * i * 0.1) + ch)
    result = mix(*layers)
    return reverb(result, 0.35, [23, 41, 59])

def matcha_error():
    """错误 — 薄荷警示 'doo~doo' """
    # Dissonant interval for alert feel
    a = harmonics(330, 120, [1, 0.4, 0.2], 0.28)  # E4
    b = harmonics(311, 120, [1, 0.4, 0.2], 0.22)  # Eb4 - dissonant
    gap = [0.0] * int(SAMPLE_RATE * 0.05)
    c = harmonics(280, 150, [1, 0.3, 0.15], 0.25)
    return mix(a, gap + b, [0.0] * int(SAMPLE_RATE * 0.04) + gap + c)

def matcha_click():
    """点击 — 水滴 'dip' """
    # Bright water drop
    drop1 = sweep(2200, 1800, 25, 0.28, curve=0.3)
    env = envelope_adsr(len(drop1), 0.01, 0.1, 0.3, 0.5)
    return apply_env(drop1, env)

def matcha_switch():
    """切换 — 清风 'fwoosh~' """
    up = sweep(500, 1000, 60, 0.25, curve=0.5)
    down = sweep(1000, 700, 50, 0.2, curve=0.7)
    n_up = noise(30, 0.04, 3000)
    return mix(up, [0.0] * int(SAMPLE_RATE * 0.02) + down, n_up)

def matcha_popup():
    """弹窗 — 萌萌弹出 'pop~!' """
    pop = sweep(1600, 2000, 40, 0.3, curve=0.4)
    bell = sparkle([2000, 2500], 100, 0.15)
    bell = [0.0] * int(SAMPLE_RATE * 0.03) + bell
    return mix(pop, bell)

def matcha_typing():
    """打字 — 细碎精灵步 'titititi' """
    buf = []
    for _ in range(7):
        click = sweep(random.uniform(2200, 3200), random.uniform(1800, 2500), 12, 0.15, curve=0.5)
        buf.extend(click)
        buf.extend([0.0] * int(SAMPLE_RATE * random.uniform(0.025, 0.045)))
    return buf

# ─── Peachy 桃气 — 温暖男声软萌感 ───
# 中音域、温暖、桃橙色感、男声气质
# 音色：圆润泛音 + 弹性物理感 + 柔和

def peachy_incoming():
    """收到消息 — 温暖男声双音 'mhm~hey' """
    # Warm vowel-like sounds
    a = vibrato(440, 5, 0.025, 130, 0.28)  # A4 warm
    b_delay = [0.0] * int(SAMPLE_RATE * 0.06)
    b = vibrato(554, 4, 0.02, 100, 0.22)  # C#5 bright
    # Add warmth - sub harmonics
    sub = sine(220, 100, 0.08)
    return mix(a, b_delay + b, sub)

def peachy_sent():
    """发送消息 — 弹性发送 'pop~' """
    b1 = bounce(480, 2, 80, 0.28)
    # Add click
    click = sweep(1200, 800, 20, 0.15)
    return mix(b1, click)

def peachy_success():
    """成功 — 温暖欢呼 'ya~yay!' """
    notes = [392, 494, 587, 784]  # G4-B4-D5-G5
    layers = []
    for i, f in enumerate(notes):
        t = vibrato(f, 3.5, 0.015, 180, 0.2)
        # Add warm sub
        sub = sine(f * 0.5, 100, 0.06)
        t = mix(t, sub)
        layers.append([0.0] * int(SAMPLE_RATE * i * 0.1) + t)
    result = mix(*layers)
    return reverb(result, 0.3, [29, 47])

def peachy_error():
    """错误 — 温柔叹气 'aww~' """
    # Descending warm tone
    desc = sweep(320, 240, 180, 0.28, curve=0.8)
    # Add breathy texture
    breath = noise(100, 0.03, 2000)
    breath = [0.0] * int(SAMPLE_RATE * 0.05) + breath
    return mix(desc, breath)

def peachy_click():
    """点击 — 柔和弹跳 'bop' """
    b = bounce(1100, 1, 30, 0.22)
    return b

def peachy_switch():
    """切换 — 温暖翻页 'fwap~' """
    up = sweep(350, 650, 50, 0.22, curve=0.6)
    n = noise(25, 0.05, 2500)
    return mix(up, n)

def peachy_popup():
    """弹窗 — 桃心弹出 'bwom~!' """
    boing = bounce(750, 2, 100, 0.28)
    # Add sparkle
    sp = sparkle([1100], 60, 0.1)
    sp = [0.0] * int(SAMPLE_RATE * 0.04) + sp
    return mix(boing, sp)

def peachy_typing():
    """打字 — 温暖按键 'tuputupu' """
    buf = []
    for _ in range(6):
        f = random.uniform(550, 850)
        click = bounce(f, 1, 15, 0.12)
        buf.extend(click)
        buf.extend([0.0] * int(SAMPLE_RATE * random.uniform(0.035, 0.06)))
    return buf

# ─── Mochi 糯 — 软糯宝宝萌感 ───
# 低中音域、柔软、米白色感、宝宝/萌宝气质
# 音色：圆润低频 + 慢速颤音 + 棉花糖质感

def mochi_incoming():
    """收到消息 — 软糯呼唤 'nuo~nuo~nuo' """
    # Soft, round, wobbly
    a = vibrato(330, 2.5, 0.05, 200, 0.25)  # E4 soft
    b = [0.0] * int(SAMPLE_RATE * 0.08) + vibrato(392, 2, 0.04, 170, 0.22)  # G4
    c = [0.0] * int(SAMPLE_RATE * 0.18) + vibrato(440, 2, 0.03, 150, 0.18)  # A4
    # Sub warmth
    sub = sine(165, 250, 0.06)  # Deep warmth
    return mix(a, b, c, sub)

def mochi_sent():
    """发送消息 — 软软弹出 'puo~' """
    b = bounce(320, 3, 130, 0.22)
    # Soft click
    click = sweep(800, 600, 20, 0.1)
    return mix(b, click)

def mochi_success():
    """成功 — 萌宝欢呼 'wa~wah~!' """
    notes = [262, 330, 392, 523]  # C4-E4-G4-C5
    layers = []
    for i, f in enumerate(notes):
        t = vibrato(f, 2, 0.04, 220, 0.18)
        # Add sub warmth
        sub = sine(f * 0.5, 150, 0.05)
        t = mix(t, sub)
        layers.append([0.0] * int(SAMPLE_RATE * i * 0.12) + t)
    result = mix(*layers)
    return reverb(result, 0.25, [31, 53])

def mochi_error():
    """错误 — 软萌沮丧 'uu~' """
    # Low, trembling, sad
    t = vibrato(220, 7, 0.06, 250, 0.25)
    # Wobble on top
    wobble = sweep(250, 200, 200, 0.08, curve=1.5)
    return mix(t, wobble)

def mochi_click():
    """点击 — 糯米团轻触 'nuk' """
    # Soft, round, cotton-like
    a = sweep(700, 500, 35, 0.15, curve=0.5)
    sub = sine(350, 25, 0.06)
    return mix(a, sub)

def mochi_switch():
    """切换 — 糯米翻滚 'gululu~' """
    up = sweep(250, 480, 80, 0.2, curve=0.6)
    wobble = vibrato(400, 10, 0.08, 60, 0.1)
    return mix(up, wobble)

def mochi_popup():
    """弹窗 — 糯米弹弹 'bom~!' """
    b = bounce(450, 3, 160, 0.22)
    # Soft high accent
    accent = vibrato(660, 4, 0.03, 80, 0.1)
    accent = [0.0] * int(SAMPLE_RATE * 0.03) + accent
    return mix(b, accent)

def mochi_typing():
    """打字 — 糯米步 'nuonuo' """
    buf = []
    for _ in range(5):
        f = random.uniform(400, 650)
        click = sweep(f, f * 0.8, 20, 0.08, curve=0.6)
        buf.extend(click)
        buf.extend([0.0] * int(SAMPLE_RATE * random.uniform(0.04, 0.07)))
    return buf

# ── 写入 WAV ──────────────────────────────────────────────

def write_wav(path, samples, sample_rate=SAMPLE_RATE):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    samples = normalize(samples, 0.82)
    fade_in(samples, 200)
    fade_out(samples, 600)
    
    with wave.open(path, 'w') as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)  # 16-bit
        wf.setframerate(sample_rate)
        for s in samples:
            val = max(-1, min(1, s))
            wf.writeframes(struct.pack('<h', int(val * 32767)))
    
    dur_ms = len(samples) * 1000 / sample_rate
    kb = os.path.getsize(path) // 1024
    print(f"  ✓ {os.path.basename(path):20s}  {dur_ms:6.0f}ms  {kb:>3d}KB")


# ── 主流程 ────────────────────────────────────────────────

def main():
    base = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'assets', 'sounds')
    
    themes = {
        'matcha': {
            'msg_incoming': matcha_incoming,
            'msg_sent':     matcha_sent,
            'success':      matcha_success,
            'error':        matcha_error,
            'click':        matcha_click,
            'switch':       matcha_switch,
            'popup':        matcha_popup,
            'typing':       matcha_typing,
        },
        'peachy': {
            'msg_incoming': peachy_incoming,
            'msg_sent':     peachy_sent,
            'success':      peachy_success,
            'error':        peachy_error,
            'click':        peachy_click,
            'switch':       peachy_switch,
            'popup':        peachy_popup,
            'typing':       peachy_typing,
        },
        'mochi': {
            'msg_incoming': mochi_incoming,
            'msg_sent':     mochi_sent,
            'success':      mochi_success,
            'error':        mochi_error,
            'click':        mochi_click,
            'switch':       mochi_switch,
            'popup':        mochi_popup,
            'typing':       mochi_typing,
        },
    }
    
    print("🎵 AnyClaw LVGL — 萌萌音效生成器 v2")
    print("   泛音 · 共振峰 · 混响 · 颤音 · 弹性物理")
    print("=" * 58)
    
    total = 0
    theme_chars = {
        'matcha': '🌿 清新女声（薄荷绿）',
        'peachy': '🍑 温暖男声（蜜桃橙）',
        'mochi':  '🍡 软糯萌宝（糯米白）',
    }
    
    for theme, sounds in themes.items():
        print(f"\n{theme_chars[theme]}")
        print("-" * 40)
        for name, gen_fn in sounds.items():
            path = os.path.join(base, theme, f'{name}.wav')
            write_wav(path, gen_fn())
            total += 1
    
    print(f"\n{'=' * 58}")
    print(f"✅ 完成！生成 {total} 个音效文件")
    print(f"📁 输出目录: {os.path.abspath(base)}")


if __name__ == '__main__':
    random.seed(42)
    main()
