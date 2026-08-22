#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
check_build_env.py — 诊断 CMake + GCC + OpenOCD 构建环境（只读检查，不安装任何软件）

用法:
    python check_build_env.py [--required-gcc <path>] [--required-openocd <path>]

exit code: 0 = 必需工具齐全; 1 = 有必需工具缺失
"""

import argparse
import shutil
import subprocess
import sys
from typing import Dict, List, Optional, Tuple


def probe_tool(name: str, path: Optional[str] = None, ver_args: List[str] = None) -> Tuple[bool, str]:
    """尝试运行工具获取版本号。path 为空时在 PATH 中查找。"""
    exe = path or shutil.which(name)
    if not exe:
        return False, "未找到"
    try:
        out = subprocess.run(
            [exe] + (ver_args or ["--version"]),
            capture_output=True, text=True, timeout=15,
        )
        first = (out.stdout or out.stderr).strip().splitlines()
        return True, (first[0][:70] if first else "可用")
    except (OSError, subprocess.TimeoutExpired) as e:
        return False, f"运行失败: {e}"


def main() -> int:
    ap = argparse.ArgumentParser(description="CMake/GCC/OpenOCD 构建环境诊断")
    ap.add_argument("--required-gcc", help="arm-none-eabi-gcc 绝对路径（可选）")
    ap.add_argument("--required-openocd", help="openocd 绝对路径（可选）")
    args = ap.parse_args()

    required: Dict[str, Tuple[Optional[str], List[str]]] = {
        "cmake": (None, ["--version"]),
        "ninja": (None, ["--version"]),
        "arm-none-eabi-gcc": (args.required_gcc, ["--version"]),
        "arm-none-eabi-objcopy": (None, ["--version"]),
        "arm-none-eabi-size": (None, ["--version"]),
        "arm-none-eabi-gdb": (None, ["--version"]),
    }
    optional: Dict[str, Tuple[Optional[str], List[str]]] = {
        "openocd": (args.required_openocd, ["--version"]),
        "pyocd": (None, ["--version"]),
    }

    print("== 必需工具 ==")
    missing = []
    for name, (path, args_) in required.items():
        ok, info = probe_tool(name, path, args_)
        mark = "[OK ]" if ok else "[MISSING]"
        print(f"  {mark} {name:<22} {info}")
        if not ok:
            missing.append(name)

    print("== 可选工具（烧录/调试）==")
    for name, (path, args_) in optional.items():
        ok, info = probe_tool(name, path, args_)
        mark = "[OK ]" if ok else "[-- ]"
        print(f"  {mark} {name:<22} {info}")

    if missing:
        print(f"\n缺少必需工具: {', '.join(missing)}")
        print("本脚本仅诊断。请自行安装缺失工具后再运行构建。")
        return 1

    print("\n必需工具齐全，可继续 CMake 配置与构建。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
