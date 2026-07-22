const std = @import("std");

fn fail(comptime fmt: []const u8, args: anytype, ret_addr: usize) noreturn {
    std.debug.print("assertion failed: ", .{});
    std.debug.print(fmt, args);
    std.debug.print("\n", .{});
    std.debug.dumpCurrentStackTrace(.{ .first_address = ret_addr });
    std.process.exit(1);
}

export fn zig_assert_int_equal(expect: usize, actual: usize) void {
    if (expect != actual) {
        fail("{d} != {d}", .{ expect, actual }, @returnAddress());
    }
}

export fn zig_assert_true(result: usize, condition: [*:0]const u8) void {
    if (result == 0) {
        fail("{s}", .{condition}, @returnAddress());
    }
}

export fn zig_assert_memory_equal(a: [*]const u8, b: [*]const u8, size: usize) void {
    if (!std.mem.eql(u8, a[0..size], b[0..size])) {
        fail("memory is not equal", .{}, @returnAddress());
    }
}
