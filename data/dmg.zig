const std = @import("std");

export const ops_data = @embedFile("c_ops");
comptime {
    std.debug.assert(ops_data.len == 4096);
}

export const cb_ops_data = @embedFile("c_cb_ops");
comptime {
    std.debug.assert(cb_ops_data.len == 4096);
}

export const boot_rom_data = @embedFile("boot_rom");
export const boot_rom_size = boot_rom_data.len;
comptime {
    std.debug.assert(boot_rom_size == 256);
}
