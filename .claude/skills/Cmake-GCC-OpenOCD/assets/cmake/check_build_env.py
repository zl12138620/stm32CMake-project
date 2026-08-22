#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
check_build_env.py — 诊断 CMake + GCC + OpenOCD 构建环境（只读检查，不安装任何软件）

用法:
    python check_build_env.py
    python check_build_env.py --gcc-path <arm-none-eabi-gcc.exe路径>
    python check_build_env.py --openocd-path <openocd.exe路径> --pyocd-path <pyocd.exe路径>

每个工具都可通过 `--<name>-path` 显式指定路径；未指定时在 PATH 中查找。

exit code: 0 = 必需工具齐全; 1 = 有必需工具缺失
"""

import argparse
import shutil
import subprocess
import sys
from typing import Dict, List, Optional, Tuple

# 工具名 -> (参数名, 版本参数)
TOOLS = {
    "cmake": ("cmake", ["--version"]),
    "ninja": ("ninja", ["--version"]),
    "arm-none-eabi-gcc": ("gcc", ["--version"]),
    "arm-none-eabi-objcopy": ("objcopy", ["--version"]),
    "arm-none-eabi-size": ("size", ["--version"]),
    "arm-none-eabi-gdb": ("gdb", ["--version"]),
}
OPTIONAL_TOOLS = {
    "openocd": ("openocd", ["--version"]),
    "pyocd": ("pyocd", ["--version"]),
}


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
    for tool, (arg_name, _) in TOOLS.items():
        ap.add_argument(f"--{arg_name}-path", default=None,
                        help=f"{tool} 绝对路径（可选，未指定时在 PATH 中查找）")
    for tool, (arg_name, _) in OPTIONAL_TOOLS.items():
        ap.add_argument(f"--{arg_name}-path", default=None,
                        help=f"{tool} 绝对路径（可选）")
    # 兼容旧参数
    ap.add_argument("--required-gcc", default=None, help="(兼容) arm-none-eabi-gcc 绝对路径")
    ap.add_argument("--required-openocd", default=None, help="(兼容) openocd 绝对路径")
    args = ap.parse_args()

    def path_of(arg_name: str) -> Optional[str]:
        return getattr(args, f"{arg_name}_path", None)

    required: Dict[str, Tuple[Optional[str], List[str]]] = {
        tool: (path_of(arg_name), ver) for tool, (arg_name, ver) in TOOLS.items()
    }
    optional: Dict[str, Tuple[Optional[str], List[str]]] = {
        tool: (path_of(arg_name), ver) for tool, (arg_name, ver) in OPTIONAL_TOOLS.items()
    }
    # 兼容旧参数覆盖
    if args.required_gcc:
        required["arm-none-eabi-gcc"] = (args.required_gcc, ["--version"])
    if args.required_openocd:
        optional["openocd"] = (args.required_openocd, ["--version"])

    print("== 必需工具 ==")
    missing = []
    for name, (path, ver_args) in required.items():
        ok, info = probe_tool(name, path, ver_args)
        mark = "[OK ]" if ok else "[MISSING]"
        print(f"  {mark} {name:<22} {info}")
        if not ok:
            missing.append(name)

    print("== 可选工具（烧录/调试）==")
    for name, (path, ver_args) in optional.items():
        ok, info = probe_tool(name, path, ver_args)
        mark = "[OK ]" if ok else "[-- ]"
        print(f"  {mark} {name:<22} {info}")

    if missing:
        print(f"\n缺少必需工具: {', '.join(missing)}")
        print("本脚本仅诊断。请提供已安装工具的路径（--<name>-path）后重新检测，或自行安装。")
        return 1

    print("\n必需工具齐全，可继续 CMake 配置与构建。")
    return 0


if __name__ == "__main__":
    sys.exit(main())

