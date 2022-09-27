#!/usr/bin/env python3

from pathlib import Path
import sys
import os
import argparse
import subprocess

EXTENSIONS = set('.' + ext for ext in ['c', 'h', 'cpp', 'hpp', 'inl'])


def get_files(dirs):
    """
    Generator which yields all files in the given directories with any of the
    EXTENSIONS.
    """
    for dir in dirs:
        for root, _, files in os.walk(dir):
            for file in files:
                path = Path(os.path.join(root, file))
                if path.suffix in EXTENSIONS:
                    yield path


def main():
    parser = argparse.ArgumentParser(
        description="""
        Run command on all C/C++ source files in the given directories
        """)
    parser.add_argument('--dirs', type=Path, nargs='+',
                        help='Directories to search in')
    parser.add_argument('command', nargs='+',
                        help='Command to which to pass found files')
    args = parser.parse_args()

    all_files = list(str(file) for file in get_files(args.dirs))

    if not all_files:
        print("No files found")
        sys.exit(1)

    result = subprocess.run(args.command + all_files)
    print(f'Formatted {len(all_files)} files')

    if result.returncode != 0:
        sys.exit(result.returncode)


if __name__ == '__main__':
    main()
