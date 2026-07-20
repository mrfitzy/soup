const std = @import("std");

export fn zig_assert_int_equal(expect: c_int, actual: c_int) void {
    if (expect != actual) {
        std.debug.panicExtra(@returnAddress(), "{d} != {d}", .{ expect, actual });
    }
}

export fn zig_assert_true(result: c_int, condition: [*:0]const u8) void {
    if (result == 0) {
        std.debug.panicExtra(@returnAddress(), "assertion failed: {s}", .{condition});
    }
}

export fn zig_assert_memory_equal(a: [*]const u8, b: [*]const u8, size: usize) void {
    if (!std.mem.eql(u8, a[0..size], b[0..size])) {
        std.debug.panicExtra(@returnAddress(), "memory is not equal", .{});
    }
}
