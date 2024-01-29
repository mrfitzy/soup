#!/usr/bin/env python3

import os
import subprocess
import sys
from typing import IO


def handle_backtrace_line(line: str) -> None:
    tokens = line.split()
    if len(tokens) != 6:
        print(line)
        return

    # [frame_id, elf, address, symbol, '+', offset]
    frame_id = tokens[0]
    elf = tokens[1]
    symbol_plus_offset = ''.join(tokens[3:])
    print(f'{frame_id:2} {elf:25}', end='')
    if elf != os.path.basename(sys.argv[1]):
        print(f'{symbol_plus_offset}')
        return

    # addr2line frequently prints debug errors after printing correct result
    # skip these lines after obtaining the first
    args = ['addr2line', '-e', elf, '-fp', symbol_plus_offset]
    process = subprocess.Popen(args, bufsize=0, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    for line2 in iter(process.stdout.readline, b''):
        pretty = line2.decode("utf-8")[:-1]
        break
    process.wait()

    tokens = pretty.split()
    if len(tokens) != 3:
        print(pretty)
        return
    # [function, 'at', file + ':' + line]
    function = tokens[0]
    tokens = tokens[2].split(':')
    if len(tokens) != 2:
        print(pretty)
        return
    filename = os.path.basename(tokens[0])
    print(f'{function} @ {filename}:{tokens[1]}')


def handle_line(line: str, backtrace: bool, log_file: IO[any]) -> bool:
    print(line, file=log_file)
    if line == '---backtrace begin---':
        assert not backtrace
        return True
    if line == '---backtrace end---':
        assert backtrace
        return False

    if not backtrace:
        print(line)
    else:
        handle_backtrace_line(line)
    return backtrace


if __name__ == "__main__":
    # from https://web.archive.org/web/20240126174209/https://gist.github.com/JLeClerc/831d400763b7020599d9
    process = subprocess.Popen(sys.argv[1:], bufsize=0, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    with open('test_run.log', 'w+') as log_file:
        backtrace = False
        for line in iter(process.stdout.readline, b''):
            backtrace = handle_line(line.decode('utf-8')[:-1], backtrace, log_file)
        process.stdout.close()
        process.wait()
    sys.exit(process.returncode)