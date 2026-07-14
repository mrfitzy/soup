const std = @import("std");

const BuildError = error{
    InvalidCrossCompile,
};

fn buildTool(b: *std.Build, optimize: std.builtin.OptimizeMode) [2]std.Build.LazyPath {
    const tool = b.addExecutable(.{
        .name = "gen_ops",
        .root_module = b.createModule(.{
            .root_source_file = b.path("tools/gen_ops.zig"),
            .target = b.graph.host,
            .optimize = optimize,
        }),
    });
    tool.root_module.addIncludePath(b.path("src/"));

    // deps
    const args_dep = b.dependency("args", .{
        .target = b.graph.host,
        .optimize = optimize,
    });
    tool.root_module.addImport("args", args_dep.module("args"));

    // build step
    const build_step = b.step("tool", "Build gen_ops tool");
    build_step.dependOn(&b.addInstallArtifact(tool, .{}).step);

    // run step
    const run_exe = b.addRunArtifact(tool);
    run_exe.addPrefixedFileArg("--input=", b.path("data/opcodes.html"));
    const ops = run_exe.addPrefixedOutputFileArg("--output=", "ops.bin");
    const cb_ops = run_exe.addPrefixedOutputFileArg("--cb-output=", "cb_ops.bin");

    const run_step = b.step("run_tool", "Run gen_ops tool");
    run_step.dependOn(&run_exe.step);

    return .{ ops, cb_ops };
}

fn buildDataLib(b: *std.Build, ops: std.Build.LazyPath, cb_ops: std.Build.LazyPath, target: std.Build.ResolvedTarget, optimize: std.builtin.OptimizeMode) *std.Build.Step.Compile {
    const lib = b.addLibrary(.{
        .name = "dmg_data",
        .linkage = .static,
        .root_module = b.createModule(.{
            .root_source_file = b.path("data/dmg.zig"),
            .target = target,
            .optimize = optimize,
        }),
    });
    lib.root_module.addAnonymousImport("c_ops", .{ .root_source_file = ops });
    lib.root_module.addAnonymousImport("c_cb_ops", .{ .root_source_file = cb_ops });
    lib.root_module.addAnonymousImport("boot_rom", .{ .root_source_file = b.path("data/DMG_ROM.bin") });
    return lib;
}

fn buildSoup(b: *std.Build, data_lib: *std.Build.Step.Compile, target: std.Build.ResolvedTarget, optimize: std.builtin.OptimizeMode) !void {
    // sources
    const common_c_sources = &[_][]const u8{
        "src/apu.c",
        "src/cpu.c",
        "src/diag.c",
        "src/display.c",
        "src/dmg.c",
        "src/lcdc.c",
        "src/mem.c",
        "src/op.c",
        "src/regs.c",
    };
    const soup_c_sources = common_c_sources ++ .{
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
        .files = soup_c_sources,
        .flags = flags.items,
    });

    // dependencies
    soup.root_module.linkLibrary(data_lib);

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
    const run_step = b.step("run", "Run soup");
    run_step.dependOn(&run_exe.step);
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const ops, const cb_ops = buildTool(b, optimize);
    const data_lib = buildDataLib(b, ops, cb_ops, target, optimize);
    buildSoup(b, data_lib, target, optimize) catch |err| {
        std.debug.print("error building soup: {}\n", .{err});
        std.process.exit(1);
    };
}
