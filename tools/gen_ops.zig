const std = @import("std");
const args = @import("args");

const c = @cImport({
    @cInclude("op.h");
});

const skip: [11]u8 = .{
    0xD3,
    0xDB,
    0xDD,
    0xE3,
    0xE4,
    0xEB,
    0xEC,
    0xED,
    0xF4,
    0xFC,
    0xFD,
};

const GenOpsError = error{
    MissingOpField,
};

const Op = struct {
    opcode: u8,
    mnemonic: ?[]const u8 = null,
    length_duration: ?[]const u8 = null,
    flags: ?[]const u8 = null,

    pub fn parseLength(op: Op) !u8 {
        if (op.length_duration) |str| {
            return try std.fmt.parseInt(u8, str[0..std.mem.findScalar(u8, str, '&').?], 10);
        } else return error.MissingOpField;
    }

    pub fn parseDuration(op: Op) !u8 {
        if (op.length_duration) |str| {
            const duration_str = str[std.mem.findScalarLast(u8, str, ';').? + 1 ..];
            var it = std.mem.splitScalar(u8, duration_str, '/');
            var duration: u8 = try std.fmt.parseInt(u8, it.first(), 10) / 4;
            if (it.next()) |dur| {
                duration = (duration << 4) | (try std.fmt.parseInt(u8, dur, 10) / 4);
            }
            return duration;
        } else return error.MissingOpField;
    }

    pub fn parseFlags(op: Op) !u8 {
        if (op.flags) |str| {
            var it = std.mem.splitBackwardsScalar(u8, str, ' ');
            var flags: u8 = 0;
            inline for (0..4) |i| {
                var flag: u8 = 0;
                const ch = it.next().?[0];
                if (ch == @as(u8, '1')) {
                    flag = 0x1;
                } else if (ch == '0') {
                    flag = 0x2;
                } else if (ch != '-') {
                    flag = 0x3;
                }
                flags |= (flag << (2 * i));
            }
            return flags;
        } else return error.MissingOpField;
    }

    pub fn toC(op: Op) c.op {
        var c_op = std.mem.zeroes(c.op);
        c_op.opcode = op.opcode;
        c_op.length = op.parseLength() catch 0;
        c_op.duration = op.parseDuration() catch 0;
        c_op.flags = op.parseFlags() catch 0;
        if (op.mnemonic) |m| {
            @memcpy(c_op.text[0..m.len], m);
        }
        return c_op;
    }
};

fn parseOps(lines: []const []const u8, ops: []Op) void {
    var n: usize = 0;
    for (lines, 0..) |line, hi| {
        var i: usize = 0;
        for (0..16) |lo| {
            var op: Op = .{ .opcode = @intCast((hi << 4) | lo) };
            if (std.mem.findScalar(u8, &skip, op.opcode)) |_| {
                // skip over '<td class="withborder">&nbsp;</td>'
                i += 34;
                ops[n] = op;
                n += 1;
                continue;
            }
            if (op.opcode == 0xCB) {
                // special case: skip entire cell
                i += 84;
                ops[n] = op;
                n += 1;
                continue;
            }
            var labels: [3][]const u8 = undefined;
            for (0..3) |x| {
                i += std.mem.findScalar(u8, line[i..], '>').? + 1;
                const next: usize = i + std.mem.findScalar(u8, line[i..], '<').?;
                labels[x] = line[i..next];
                i = next + 1;
            }
            op.mnemonic, op.length_duration, op.flags = labels;
            ops[n] = op;
            n += 1;
            // skip over '/td>'
            i += 4;
        }
    }
}

fn parseCbOps(lines: []const []const u8, cb_ops: []Op) void {
    var n: usize = 0;
    for (lines, 0..) |line, hi| {
        var i: usize = 0;
        for (0..16) |lo| {
            var op: Op = .{ .opcode = @intCast((hi << 4) | lo) };
            var labels: [3][]const u8 = undefined;
            for (0..3) |x| {
                i += std.mem.findScalar(u8, line[i..], '>').? + 1;
                const next: usize = i + std.mem.findScalar(u8, line[i..], '<').?;
                labels[x] = line[i..next];
                i = next + 1;
            }
            op.mnemonic, op.length_duration, op.flags = labels;
            cb_ops[n] = op;
            n += 1;
            // skip over '/td>'
            i += 4;
        }
    }
}

fn writeOpsBin(io: std.Io, ops: []const Op, ops_out_path: []const u8) !void {
    //
    // 16 bytes per op
    // opcode, length, ((duration_hi << 4) | duration_lo), flags, text0, ..., text11
    //
    // flags: 7 6 5 4 3 2 1 0    for each flag:
    //        ---------------      00: unaffected
    //        | | | | | | | |      01: set after execution
    //        Z Z N N H H C C      10: cleared after execution
    //                             11: depends on result of execution
    //
    var f = try std.Io.Dir.cwd().createFile(io, ops_out_path, .{ .truncate = true });
    defer f.close(io);
    var buf: [4096]u8 = undefined;
    var w: std.Io.File.Writer = f.writer(io, &buf);
    const writer = &w.interface;
    for (ops) |op| {
        try writer.writeStruct(op.toC(), .native);
    }
    try w.flush();
}

fn createOpMetadata(allocator: std.mem.Allocator, io: std.Io, html_path: []const u8, ops_out_path: []const u8, cb_ops_out_path: []const u8) !void {
    // read html
    const html = try std.Io.Dir.cwd().readFileAlloc(io, html_path, allocator, .unlimited);
    defer allocator.free(html);
    var lines: std.ArrayList([]const u8) = .empty;
    defer lines.deinit(allocator);
    var it = std.mem.splitScalar(u8, html, '\n');
    while (it.next()) |line| {
        try lines.append(allocator, line);
    }
    lines.orderedRemoveMany(&.{ 0, 1 }); // skip comments

    var ops: [256]Op = undefined;
    parseOps(lines.items[0..16], &ops);

    var cb_ops: [256]Op = undefined;
    parseCbOps(lines.items[16..], &cb_ops);

    try writeOpsBin(io, &ops, ops_out_path);
    try writeOpsBin(io, &cb_ops, cb_ops_out_path);
}

pub fn main(init: std.process.Init) !void {
    const allocator = init.arena.allocator();
    var parser = try args.ArgumentParser.init(allocator, .{
        .name = "gen_ops",
    });
    defer parser.deinit();

    try parser.addArg(.{
        .short = 'i',
        .long = "input",
        .name = "input",
        .metavar = "input_html",
    });
    try parser.addArg(.{
        .short = 'o',
        .long = "output",
        .name = "output",
        .metavar = "ops_bin",
    });
    try parser.addArg(.{
        .short = 'c',
        .long = "cb-output",
        .name = "cb-output",
        .metavar = "cb_ops_bin",
    });

    var my_args = try parser.parseProcess(init);
    defer my_args.deinit();
    const input = my_args.getString("input") orelse "opcodes.html";
    const output = my_args.getString("output") orelse "ops.bin";
    const cb_output = my_args.getString("cb-output") orelse "cb_ops.bin";

    createOpMetadata(allocator, init.io, input, output, cb_output) catch |err| {
        std.debug.print("error generating ops data: {}\n", .{err});
        std.process.exit(1);
    };
}
