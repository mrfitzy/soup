const std = @import("std");

comptime {
    _ = @import("log.zig");
}

fn fail(comptime fmt: []const u8, args: anytype, ret_addr: usize) noreturn {
    std.debug.print("assertion failed: ", .{});
    std.debug.print(fmt, args);
    std.debug.print("\n", .{});
    std.debug.dumpCurrentStackTrace(.{ .first_address = ret_addr });
    std.process.exit(1);
}

export fn zigAssertIntEqual(expect: usize, actual: usize) void {
    if (expect != actual) {
        fail("{d} != {d}", .{ expect, actual }, @returnAddress());
    }
}

export fn zigAssertTrue(result: usize, condition: [*:0]const u8) void {
    if (result == 0) {
        fail("{s}", .{condition}, @returnAddress());
    }
}

export fn zigAssertMemoryEqual(a: [*]const u8, b: [*]const u8, size: usize) void {
    const c = a[0..size];
    const d = b[0..size];
    for (c, d, 0..) |e, f, i| {
        if (e != f) {
            fail("byte 0x{x} differs: 0x{x} != 0x{x}", .{ i, e, f }, @returnAddress());
        }
    }
}
