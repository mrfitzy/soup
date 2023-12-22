#!/usr/bin/env python3

import argparse

SKIP = frozenset({(0xd, 0x3), (0xd, 0xb), (0xd, 0xd),
                  (0xe, 0x3), (0xe, 0x4), (0xe, 0xb), (0xe, 0xc), (0xe, 0xd),
                  (0xf, 0x4), (0xf, 0xc), (0xf, 0xd)})
CB = (0xc, 0xb)


def parse_ops(lines: list[str]) -> list:
    ops = []
    for hi in range(0, 16):
        line = lines[hi]
        i = 0
        for lo in range(0, 16):
            if (hi, lo) in SKIP:
                # skip over '<td class="withborder">&nbsp;</td>'
                i += 34
                continue
            if (hi, lo) == CB:
                # special case: skip entire cell
                i += 84
                continue
            attrs = [(hi << 4) | lo]
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


def parse_opcode_html(html_path: str='opcodes.html'):
    lines = []
    with open(html_path, 'r') as f:
        lines = f.readlines()
    ops = parse_ops(lines)
    cb_ops = parse_cb_ops(lines[16:])


if __name__ == "__main__":
    # parser = argparse.ArgumentParser()
    # parser.add_argument('opcode_html')
    # args = parser.parse_args()
    # parse_opcode_html(args.opcode_html)
    parse_opcode_html()