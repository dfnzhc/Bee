#!/usr/bin/env python3
import os
import sys
import shutil
import argparse
import subprocess
from pathlib import Path

SUPPORTED_EXTENSIONS = {'.c', '.cpp', '.cc', '.cxx', '.h', '.hpp', '.hh', '.hxx'}
EXCLUDE_DIRS = {'.git', 'build', 'cmake-build-debug', 'cmake-build-release', 'out'}


def check_dependencies(skip_tidy):
    missing_tools = []
    if not shutil.which('clang-format'):
        missing_tools.append('clang-format')
    if not skip_tidy and not shutil.which('clang-tidy'):
        missing_tools.append('clang-tidy')

    if missing_tools:
        print("❌ 错误: 找不到以下程序，请确保它们已安装：")
        for tool in missing_tools:
            print(f"  - {tool}")
        sys.exit(1)


def get_source_files(target_dirs):
    """遍历所有传入的目标目录，提取源文件"""
    source_files = []

    for target_dir in target_dirs:
        path = Path(target_dir).resolve()
        if not path.exists() or not path.is_dir():
            print(f"⚠️ 警告: 目录不存在或无效，已跳过 -> {target_dir}")
            continue

        for file in path.rglob('*'):
            if any(part in EXCLUDE_DIRS for part in file.parts):
                continue
            if file.is_file() and file.suffix.lower() in SUPPORTED_EXTENSIONS:
                source_files.append(str(file))

    return source_files


def format_file(file_path):
    try:
        subprocess.run(['clang-format', '-i', '-style=file', file_path], check=True)
        return True
    except subprocess.CalledProcessError as e:
        print(f"⚠️ clang-format 失败 [{file_path}]: {e}")
        return False


def tidy_file(file_path, build_dir):
    try:
        cmd = ['clang-tidy']
        if build_dir:
            cmd.extend(['-p', str(Path(build_dir).resolve())])
        cmd.extend(['--fix', '--quiet', file_path])
        subprocess.run(cmd, check=False, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        return True
    except Exception as e:
        print(f"⚠️ clang-tidy 异常 [{file_path}]: {e}")
        return False


def main():
    parser = argparse.ArgumentParser()
    # 接收一个或多个目录
    parser.add_argument("directories", nargs='+', help="目标文件夹路径列表")
    parser.add_argument("--skip-tidy", action="store_true", help="跳过 clang-tidy")
    parser.add_argument("--build-dir", type=str, default=None, help="构建目录路径")
    args = parser.parse_args()

    check_dependencies(args.skip_tidy)

    files = get_source_files(args.directories)
    if not files:
        print("ℹ️ 在指定的目录中没有找到有效的 C/C++ 源文件。")
        sys.exit(0)

    print(f"🔍 找到 {len(files)} 个源文件，开始处理...")

    for idx, file_path in enumerate(files, 1):
        filename = os.path.basename(file_path)
        print(f"[{idx}/{len(files)}] 处理中: {filename}")

        if not args.skip_tidy:
            tidy_file(file_path, args.build_dir)

        format_file(file_path)

    print("✅ 所有文件处理完毕！")


if __name__ == "__main__":
    main()
