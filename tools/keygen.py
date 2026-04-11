#!/usr/bin/env python3
"""
AnyClaw license key generator (LIC-01).

Key formats:
- HW-<MAC12><SIGN12>
- TM-<EXPHEX8><SIGN12>
"""

import argparse
import hmac
import hashlib
import time
import re

SECRET = b"AnyClaw-2026-LOVE-GARLIC"


def sign12(payload: str) -> str:
    digest = hmac.new(SECRET, payload.encode("utf-8"), hashlib.sha256).digest()
    return digest[:6].hex().upper()


def normalize_mac(mac: str) -> str:
    s = re.sub(r"[^0-9A-Fa-f]", "", mac).upper()
    if len(s) != 12:
        raise ValueError("MAC must contain 12 hex chars")
    return s


def gen_hw(mac: str) -> str:
    mac12 = normalize_mac(mac)
    sig = sign12(f"HW|{mac12}")
    return f"HW-{mac12}{sig}"


def gen_tm(hours: int) -> str:
    if hours <= 0:
        raise ValueError("hours must be > 0")
    expiry = int(time.time()) + hours * 3600
    exp_hex = f"{expiry:08X}"[-8:]
    sig = sign12(f"TM|{exp_hex}")
    return f"TM-{exp_hex}{sig}"


def verify(key: str) -> str:
    k = key.strip().upper().replace(" ", "").replace("-", "")
    if k.startswith("HW"):
        body = k[2:]
        if len(body) != 24:
            return "invalid HW key length"
        mac12 = body[:12]
        sig = body[12:]
        exp = sign12(f"HW|{mac12}")
        if exp != sig:
            return "invalid HW signature"
        return f"valid HW key for MAC {mac12}"
    if k.startswith("TM"):
        body = k[2:]
        if len(body) != 20:
            return "invalid TM key length"
        exp_hex = body[:8]
        sig = body[8:]
        exp = sign12(f"TM|{exp_hex}")
        if exp != sig:
            return "invalid TM signature"
        expiry = int(exp_hex, 16)
        remain = expiry - int(time.time())
        if remain <= 0:
            return f"expired TM key (expiry={expiry})"
        return f"valid TM key ({remain // 3600}h remaining, expiry={expiry})"
    return "unknown key prefix (need HW-/TM-)"


def main() -> None:
    parser = argparse.ArgumentParser(description="AnyClaw keygen")
    sub = parser.add_subparsers(dest="cmd")

    p_hw = sub.add_parser("hw", help="generate HW key")
    p_hw.add_argument("mac", help="machine MAC (AA:BB:CC:DD:EE:FF)")

    p_tm = sub.add_parser("tm", help="generate TM key")
    p_tm.add_argument("hours", type=int, help="valid hours")

    parser.add_argument("--verify", dest="verify_key", help="verify a key")

    args = parser.parse_args()

    if args.verify_key:
        print(verify(args.verify_key))
        return
    if args.cmd == "hw":
        print(gen_hw(args.mac))
        return
    if args.cmd == "tm":
        print(gen_tm(args.hours))
        return

    parser.print_help()


if __name__ == "__main__":
    main()
