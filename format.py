#!/usr/bin/env python3
import sys
import argparse
import subprocess
from pathlib import Path

# ==========================================
# 🎯 在这里配置你的默认目标目录
# ==========================================
DEFAULT_TARGETS = ["Bee", "Tests"]


def main():
    parser = argparse.ArgumentParser(description="代码格式化与静态检查入口脚本")
    parser.add_argument("-b", "--build", type=str, help="构建目录路径 (提供此项将执行 clang-tidy，否则跳过)")
    # 修改为可接收多个目录的参数，并设为可选
    parser.add_argument("-t", "--targets", nargs='+', help="要格式化的目标目录 (不指定则使用脚本内置的默认目录)")

    args = parser.parse_args()

    # 如果用户没有通过命令行传 -t，就使用上方配置的默认目录
    targets = args.targets if args.targets else DEFAULT_TARGETS

    root_dir = Path(__file__).resolve().parent
    script_path = root_dir / "scripts" / "format_code.py"

    if not script_path.exists():
        print(f"❌ 错误: 找不到核心脚本 {script_path}")
        sys.exit(1)

    # 准备基础命令，把所有目标目录都传给底层脚本
    cmd = [sys.executable, str(script_path)] + targets

    if args.build:
        print(f"🚀 检测到构建目录 '{args.build}'，将执行 clang-tidy 检查和 clang-format 排版...")
        cmd.extend(["--build-dir", args.build])
    else:
        print("⚡ 未提供构建目录，跳过 clang-tidy，仅执行 clang-format 快速排版...")
        cmd.append("--skip-tidy")

    try:
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError:
        print("\n❌ 格式化过程中发生错误，进程中止。")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\n⚠️ 用户手动中断。")
        sys.exit(1)


if __name__ == "__main__":
    main()
