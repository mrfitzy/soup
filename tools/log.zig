const std = @import("std");

export fn zigLogDebug(msg: [*:0]const u8) void {
    std.log.debug("{s}", .{msg});
}

export fn zigLogError(msg: [*:0]const u8) void {
    std.log.err("{s}", .{msg});
}
