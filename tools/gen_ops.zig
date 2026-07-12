const std = @import("std");
const args = @import("args");

fn createOpMetadata(allocator: std.mem.Allocator, io: std.Io, html_path: []const u8, ops_out_path: []const u8, cb_ops_out_path: []const u8, verbose: bool) !void {
    var lines: std.ArrayList([]const u8) = .empty;
    defer lines.deinit(allocator);
    if (std.Io.Dir.cwd().openFile(io, html_path, .{
        .mode = .read_only,
    })) |f| {
        defer f.close(io);
        var buf: [2048]u8 = undefined;
        var reader: std.Io.File.Reader = f.reader(io, &buf);
        while (try reader.interface.takeDelimiter('\n')) |line| {
            try lines.append(allocator, line);
        }
    } else |err| return err;

    std.debug.print("ops_out_path: {s}\n", .{ops_out_path});
    std.debug.print("cb_ops_out_path: {s}\n", .{cb_ops_out_path});
    std.debug.print("verbose: {}\n", .{verbose});
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
        .default = "opcodes.html",
    });
    try parser.addArg(.{
        .short = 'o',
        .long = "output",
        .name = "output",
        .metavar = "ops_bin",
        .default = "ops.bin",
    });
    try parser.addArg(.{
        .short = 'c',
        .long = "cb-output",
        .name = "cb_output",
        .metavar = "cb_ops_bin",
        .default = "cb_ops.bin",
    });
    try parser.addFlag("verbose", .{
        .short = 'v',
    });

    var my_args = try parser.parseProcess(init);
    defer my_args.deinit();
    const input = my_args.getString("input") orelse "opcodes.html";
    const output = my_args.getString("output") orelse "ops.bin";
    const cb_output = my_args.getString("cb_output") orelse "cb_ops.bin";
    const verbose = my_args.getBool("verbose") orelse false;

    createOpMetadata(allocator, init.io, input, output, cb_output, verbose) catch |err| {
        std.debug.print("error generating ops data: {}\n", .{err});
        std.process.exit(1);
    };
}
