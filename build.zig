const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    //
    // main program
    //

    // sources
    const common_sources = &[_][]const u8{
        "src/apu.c",
        "src/args.c",
        "src/diag.c",
        "src/display.c",
        "src/dmg.c",
        "src/emulate.c",
        "src/lcdc.c",
        "src/mem.c",
        "src/op.c",
        "src/regs.c",
    };
    const soup_sources = common_sources ++ .{
        "src/main.c",
        "src/ui_loop.c",
    };

    // flags
    const flags = &[_][]const u8{
        "-std=c23",
        "-g",
        "-Werror",
        "-Wall",
        "-Wextra",
    };

    // executable
    const soup = b.addExecutable(.{
        .name = "soup",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    soup.root_module.addCSourceFiles(.{
        .files = soup_sources,
        .flags = flags,
    });

    // dependencies
    const sdl = b.dependency("sdl", .{
        .target = target,
        .optimize = optimize,
    });
    soup.root_module.linkLibrary(sdl.artifact("SDL3"));

    // install
    b.installArtifact(soup);

    // run step
    const run_exe = b.addRunArtifact(soup);
    run_exe.addFileArg(b.path("data/ops.bin"));
    run_exe.addFileArg(b.path("data/cb_ops.bin"));
    run_exe.addFileArg(b.path("data/DMG_ROM.bin"));

    const run_step = b.step("run", "Run soup");
    run_step.dependOn(&run_exe.step);
}
