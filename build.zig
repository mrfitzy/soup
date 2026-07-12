const std = @import("std");

const BuildError = error{
    InvalidCrossCompile,
};

fn buildTool(b: *std.Build, optimize: std.builtin.OptimizeMode) void {
    const tool = b.addExecutable(.{
        .name = "gen_ops",
        .root_module = b.createModule(.{
            .root_source_file = b.path("tools/gen_ops.zig"),
            .target = b.graph.host,
            .optimize = optimize,
        }),
        .use_llvm = true,
    });
    tool.root_module.addIncludePath(b.path("src/"));

    // deps
    const args_dep = b.dependency("args", .{
        .target = b.graph.host,
        .optimize = optimize,
    });
    tool.root_module.addImport("args", args_dep.module("args"));
    b.installArtifact(tool);

    // build step
    const build_step = b.step("tool", "Build gen_ops tool");
    build_step.dependOn(&b.addInstallArtifact(tool, .{}).step);

    // run step
    const run_exe = b.addRunArtifact(tool);
    run_exe.addPrefixedFileArg("--input=", b.path("data/opcodes.html"));
    run_exe.addPrefixedFileArg("--output=", b.path("data/ops.bin"));
    run_exe.addPrefixedFileArg("--cb-output=", b.path("data/cb_ops.bin"));

    const run_step = b.step("run_tool", "Run gen_ops tool");
    run_step.dependOn(&run_exe.step);
}

fn buildSoup(b: *std.Build, target: std.Build.ResolvedTarget, optimize: std.builtin.OptimizeMode) !void {
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
    var flags: std.ArrayList([]const u8) = .empty;
    defer flags.deinit(b.allocator);
    try flags.appendSlice(b.allocator, &[_][]const u8{
        "-std=c23",
        "-g",
        "-Werror",
        "-Wall",
        "-Wextra",
    });

    if (target.result.os.tag == .macos) {
        try flags.append(b.allocator, "-Wno-error=deprecated-declarations");
    }

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
        .flags = flags.items,
    });

    // dependencies
    const sdl = b.dependency("sdl", .{
        .target = target,
        .optimize = optimize,
    });
    const sdl_artifact = sdl.artifact("SDL3");
    soup.root_module.linkLibrary(sdl_artifact);

    if (target.result.os.tag == .macos and b.graph.host.result.os.tag != .macos) {
        if (b.graph.host.result.os.tag == .windows) {
            // MacOS SDK paths are incompatible with windows hosts
            std.log.err("error: cross-compiling for macOS from Windows is not supported", .{});
            return error.InvalidCrossCompile;
        }
        if (b.lazyDependency("macos-sdk", .{
            .target = target,
            .optimize = optimize,
        })) |sdk| {
            const macos_sdk_framework_dir = sdk.path("MacOSX26.5.sdk/System/Library/Frameworks");
            soup.root_module.addFrameworkPath(macos_sdk_framework_dir);
            sdl_artifact.root_module.addFrameworkPath(macos_sdk_framework_dir);

            const macos_sdk_lib_dir = sdk.path("MacOSX26.5.sdk/usr/lib");
            soup.root_module.addLibraryPath(macos_sdk_lib_dir);
            sdl_artifact.root_module.addLibraryPath(macos_sdk_lib_dir);

            const macos_sdk_include_dir = sdk.path("MacOSX26.5.sdk/usr/include");
            soup.root_module.addSystemIncludePath(macos_sdk_include_dir);
            sdl_artifact.root_module.addSystemIncludePath(macos_sdk_include_dir);

            soup.root_module.linkSystemLibrary("iconv", .{ .use_pkg_config = .no });
            sdl_artifact.root_module.linkSystemLibrary("iconv", .{ .use_pkg_config = .no });
        }
    }

    // install
    b.installArtifact(soup);

    // build step
    const build_step = b.step("soup", "Build soup");
    build_step.dependOn(&b.addInstallArtifact(soup, .{}).step);

    // run step
    const run_exe = b.addRunArtifact(soup);
    run_exe.addFileArg(b.path("data/ops.bin"));
    run_exe.addFileArg(b.path("data/cb_ops.bin"));
    run_exe.addFileArg(b.path("data/DMG_ROM.bin"));

    const run_step = b.step("run", "Run soup");
    run_step.dependOn(&run_exe.step);
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    buildTool(b, optimize);
    buildSoup(b, target, optimize) catch |err| {
        std.debug.print("error building soup: {}\n", .{err});
        std.process.exit(1);
    };
}
