#!/usr/bin/env python3

import argparse
import struct

SKIP = frozenset({(0xd, 0x3), (0xd, 0xb), (0xd, 0xd),
                  (0xe, 0x3), (0xe, 0x4), (0xe, 0xb), (0xe, 0xc), (0xe, 0xd),
                  (0xf, 0x4), (0xf, 0xc), (0xf, 0xd)})
CB = (0xc, 0xb)


def parse_ops(lines: list[str]) -> list:
    # opcode, [mnemonic_str, duration_str, flags_str]
    ops = []
    for hi in range(0, 16):
        line = lines[hi]
        i = 0
        for lo in range(0, 16):
            attrs = [(hi << 4) | lo]
            if (hi, lo) in SKIP:
                # skip over '<td class="withborder">&nbsp;</td>'
                i += 34
                ops.append(attrs)
                continue
            if (hi, lo) == CB:
                # special case: skip entire cell
                i += 84
                ops.append(attrs)
                continue
            for _ in range(0, 3):
                i = line.find('>', i) + 1
                next = line.find('<', i)
                attrs.append(line[i:next])
                i = next + 1
            ops.append(attrs)
            # skip over '/td>'
            i += 4
    return ops


def parse_cb_ops(lines: list[str]) -> list:
    cb_ops = []
    for hi in range(0, 16):
        line = lines[hi]
        i = 0
        for lo in range(0, 16):
            attrs = [(hi << 4) | lo]
            for _ in range(0, 3):
                i = line.find('>', i) + 1
                next = line.find('<', i)
                attrs.append(line[i:next])
                i = next + 1
            cb_ops.append(attrs)
            # skip over '/td>'
            i += 4
    return cb_ops


def write_ops_bin(ops: list, ops_out_path: str, verbose: bool):
    # opcode, length, (duration_hi << 4) | duration_lo
    with open(ops_out_path, 'wb') as f:
        for op in ops:
            opcode = op[0].to_bytes(1, 'big')
            if len(op) == 1:
                # special case: unsupported / special opcode
                metadata = struct.pack('ccc', opcode, b'\x00', b'\x00')
                if verbose:
                    print(metadata)
                f.write(metadata)
                continue
            length = int(op[2][:op[2].find('&')]).to_bytes(1, 'big')
            duration_str = op[2][op[2].rfind(';')+1:]
            durations = duration_str.split('/')
            duration = int(durations[0]) // 4
            if len(durations) == 2:
                duration = (duration << 4) | (int(durations[-1]) // 4)
            duration = duration.to_bytes(1, 'big')
            metadata = struct.pack('ccc', opcode, length, duration)
            if verbose:
                print(metadata)
            f.write(metadata)


def write_cb_ops_bin(cb_ops: list, cb_ops_out_path: str, verbose: bool):
    with open(cb_ops_out_path, 'wb') as f:
        for op in cb_ops:
            opcode = op[0].to_bytes(1, 'big')
            length = int(op[2][:op[2].find('&')]).to_bytes(1, 'big')
            duration = (int(op[2][op[2].rfind(';')+1:]) // 4).to_bytes(1, 'big')
            metadata = struct.pack('ccc', opcode, length, duration)
            if verbose:
                print(metadata)
            f.write(metadata)


def create_op_metadata(html_path: str, ops_out_path: str, cb_ops_out_path: str, verbose: bool):
    lines = []
    with open(html_path, 'r') as f:
        lines = f.readlines()[1:]
    ops = parse_ops(lines)
    cb_ops = parse_cb_ops(lines[16:])
    write_ops_bin(ops, ops_out_path, verbose)
    if verbose:
        print()
    write_cb_ops_bin(cb_ops, cb_ops_out_path, verbose)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input', metavar='intput_html', default='opcodes.html')
    parser.add_argument('-o', '--output', metavar='ops_bin', default='ops.bin')
    parser.add_argument('-c', '--cb-output', metavar='cb_ops_bin', default='cb_ops.bin')
    parser.add_argument('-v', '--verbose', action='store_true')
    args = parser.parse_args()
    create_op_metadata(args.input, args.output, args.cb_output, args.verbose)
