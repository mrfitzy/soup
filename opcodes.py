#!/usr/bin/env python3

import argparse
import binascii

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


def parse_opcode(op: list) -> bytes:
    return b'%c' % op[0]


def parse_length(op: list) -> bytes:
    return b'%c' % int(op[2][:op[2].find('&')])


def parse_duration(op: list) -> bytes:
    duration_str = op[2][op[2].rfind(';')+1:]
    durations = duration_str.split('/')
    duration = int(durations[0]) // 4
    if len(durations) == 2:
        duration = (duration << 4) | (int(durations[-1]) // 4)
    return b'%c' % duration


def parse_flags(op: list) -> bytes:
    flag_chars = op[3].split(' ')
    flags = 0
    for i in range(0, 4):
        flag = 0
        c = flag_chars[-(i+1)]
        if c == '1':
            flag = 0x1
        elif c == '0':
            flag = 0x2
        elif c != '-':
            flag = 0x3
        flags |= (flag << (2*i))
    return b'%c' % flags


def parse_text(op: list) -> bytes:
    text = bytearray()
    text.extend(op[1].encode('ascii'))
    text.extend(b'\x00' * (12 - len(text)))
    return text


def write_ops_bin(ops: list, ops_out_path: str, verbose: bool):
    '''
    16 bytes per op
    opcode, length, ((duration_hi << 4) | duration_lo), flags, text0, ..., text11

    flags: 7 6 5 4 3 2 1 0    for each flag:
           ---------------      00: unaffected
           | | | | | | | |      01: set after execution
           Z Z N N H H C C      10: cleared after execution
                                11: depends on result of execution
    '''
    with open(ops_out_path, 'wb') as f:
        for op in ops:
            opcode = parse_opcode(op)
            metadata = bytearray()
            metadata.extend(opcode)
            if len(op) == 1:
                # special case: unsupported / special opcode
                metadata.extend(b'\x00' * 15)
            else:
                length = parse_length(op)
                duration = parse_duration(op)
                flags = parse_flags(op)
                text = parse_text(op)
                metadata.extend(b'%c%c%c%b' % (length, duration, flags, text))
            if verbose:
                print(binascii.hexlify(metadata))
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
    write_ops_bin(cb_ops, cb_ops_out_path, verbose)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input', metavar='intput_html', default='opcodes.html')
    parser.add_argument('-o', '--output', metavar='ops_bin', default='ops.bin')
    parser.add_argument('-c', '--cb-output', metavar='cb_ops_bin', default='cb_ops.bin')
    parser.add_argument('-v', '--verbose', action='store_true')
    args = parser.parse_args()
    create_op_metadata(args.input, args.output, args.cb_output, args.verbose)
