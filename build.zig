const std = @import("std");

const BuildError = error{
    InvalidCrossCompile,
};

const BuildOptions = struct {
    profile: bool,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
};

pub fn build(b: *std.Build) void {
    // build options
    const options: BuildOptions = .{
        .profile = b.option(bool, "profile", "Enable profiler") orelse false,
        .target = b.standardTargetOptions(.{}),
        .optimize = b.standardOptimizeOption(.{}),
    };

    // data lib
    const ops, const cb_ops = buildTool(b, options);
    const data_lib = buildDataLib(b, ops, cb_ops, options);

    // profiler
    addProfiler(b, data_lib, options) catch |err| {
        std.debug.print("error adding profiler: {}\n", .{err});
        std.process.exit(1);
    };

    // soup
    buildSoup(b, data_lib, options) catch |err| {
        std.debug.print("error building soup: {}\n", .{err});
        std.process.exit(1);
    };

    // kitchen
    buildKitchen(b, data_lib, options) catch |err| {
        std.debug.print("error building kitchen: {}\n", .{err});
        std.process.exit(1);
    };
}

fn buildTool(b: *std.Build, options: BuildOptions) [2]std.Build.LazyPath {
    // tool executable
    const tool = b.addExecutable(.{
        .name = "gen_ops",
        .root_module = b.createModule(.{
            .root_source_file = b.path("tools/gen_ops.zig"),
            .target = b.graph.host,
            .optimize = options.optimize,
        }),
    });
    tool.root_module.addIncludePath(b.path("include"));

    // dependency: args
    const args_dep = b.dependency("args", .{
        .target = b.graph.host,
        .optimize = options.optimize,
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

fn buildDataLib(b: *std.Build, ops: std.Build.LazyPath, cb_ops: std.Build.LazyPath, options: BuildOptions) *std.Build.Step.Compile {
    const lib = b.addLibrary(.{
        .name = "dmg_data",
        .linkage = .static,
        .root_module = b.createModule(.{
            .root_source_file = b.path("data/dmg.zig"),
            .target = options.target,
            .optimize = options.optimize,
        }),
    });
    lib.root_module.addAnonymousImport("c_ops", .{ .root_source_file = ops });
    lib.root_module.addAnonymousImport("c_cb_ops", .{ .root_source_file = cb_ops });
    lib.root_module.addAnonymousImport("boot_rom", .{ .root_source_file = b.path("data/DMG_ROM.bin") });
    return lib;
}

fn buildAssertLib(b: *std.Build, options: BuildOptions) *std.Build.Step.Compile {
    const lib = b.addLibrary(.{
        .name = "assert",
        .linkage = .static,
        .root_module = b.createModule(.{
            .root_source_file = b.path("tools/assert.zig"),
            .target = options.target,
            .optimize = options.optimize,
        }),
    });
    return lib;
}

fn createEmulatorExe(b: *std.Build, exe_name: []const u8, data_lib: *std.Build.Step.Compile, options: BuildOptions) !*std.Build.Step.Compile {
    // executable
    const soup = b.addExecutable(.{
        .name = exe_name,
        .root_module = b.createModule(.{
            .target = options.target,
            .optimize = options.optimize,
            .link_libc = true,
        }),
    });

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
    var c_flags = try default_c_flags(b, options);
    defer c_flags.deinit(b.allocator);
    soup.root_module.addCSourceFiles(.{
        .files = common_c_sources,
        .flags = c_flags.items,
    });
    soup.root_module.addIncludePath(b.path("include"));

    // dependency: data lib
    soup.root_module.linkLibrary(data_lib);

    // dependency: assert lib
    const assert = buildAssertLib(b, options);
    soup.root_module.linkLibrary(assert);

    // dependency: SDL3
    const sdl = b.dependency("sdl", .{
        .target = options.target,
        .optimize = options.optimize,
    });
    const sdl_artifact = sdl.artifact("SDL3");
    soup.root_module.linkLibrary(sdl_artifact);

    // dependency: macOS sdk
    if (options.target.result.os.tag == .macos and b.graph.host.result.os.tag != .macos) {
        if (b.graph.host.result.os.tag == .windows) {
            // MacOS SDK paths are incompatible with windows hosts
            std.log.err("error: cross-compiling for macOS from Windows is not supported", .{});
            return error.InvalidCrossCompile;
        }
        if (b.lazyDependency("macos-sdk", .{
            .target = options.target,
            .optimize = options.optimize,
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

    // optional dependency: profiler (C macros only)
    if (options.profile) {
        if (b.lazyDependency("tracy", .{
            .target = options.target,
            .optimize = options.optimize,
        })) |tracy| {
            const tracy_client = tracy.artifact("tracyclient");
            soup.root_module.addIncludePath(tracy_client.getEmittedIncludeTree());
        }
    }

    return soup;
}

fn addProfiler(b: *std.Build, data_lib: *std.Build.Step.Compile, options: BuildOptions) !void {
    data_lib.root_module.addIncludePath(b.path("include"));

    if (!options.profile) {
        // use stubs
        var c_flags = try default_c_flags(b, options);
        defer c_flags.deinit(b.allocator);
        data_lib.root_module.addCSourceFiles(.{
            .files = &.{"tools/profiler_stubs.c"},
            .flags = c_flags.items,
        });
        data_lib.root_module.link_libc = true;
        return;
    }

    // XXX piggybacking on data_lib
    var cpp_flags = try default_cpp_flags(b, options);
    defer cpp_flags.deinit(b.allocator);
    data_lib.root_module.addCSourceFiles(.{
        .files = &.{"tools/profiler.cpp"},
        .flags = cpp_flags.items,
    });
    data_lib.root_module.link_libcpp = true;
    if (b.lazyDependency("tracy", .{
        .target = options.target,
        .optimize = options.optimize,
    })) |tracy| {
        const tracy_client = tracy.artifact("tracyclient");
        data_lib.root_module.linkLibrary(tracy_client);
    }
}

fn buildSoup(b: *std.Build, data_lib: *std.Build.Step.Compile, options: BuildOptions) !void {
    // make soup
    const soup = try createEmulatorExe(b, "soup", data_lib, options);

    // additional soup sources
    var c_flags = try default_c_flags(b, options);
    defer c_flags.deinit(b.allocator);
    soup.root_module.addCSourceFiles(.{
        .files = &.{
            "src/main.c",
            "src/ui_loop.c",
        },
        .flags = c_flags.items,
    });

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

pub fn buildKitchen(b: *std.Build, data_lib: *std.Build.Step.Compile, options: BuildOptions) !void {
    // kitchen debugger executable
    const kitchen = try createEmulatorExe(b, "kitchen", data_lib, options);

    // additional kitchen sources
    var c_flags = try default_c_flags(b, options);
    defer c_flags.deinit(b.allocator);
    kitchen.root_module.addCSourceFiles(.{
        .files = &.{
            "test/dmg_assert.c",
            "test/signal_handler.c",
            "test/test_main.c",
        },
        .flags = c_flags.items,
    });
    kitchen.root_module.addIncludePath(b.path("test"));

    var cpp_flags = try default_cpp_flags(b, options);
    defer cpp_flags.deinit(b.allocator);
    kitchen.root_module.addCSourceFiles(.{
        .files = &.{
            "test/ui_loop_dbg.cpp",
            "ui/tile.cpp",
            "ui/ui.cpp",
        },
        .flags = cpp_flags.items,
    });
    kitchen.root_module.link_libcpp = true;

    // additional dependencies
    kitchen.root_module.addCSourceFiles(.{
        .files = &.{
            "../imgui/backends/imgui_impl_sdl3.cpp",
            "../imgui/backends/imgui_impl_sdlrenderer3.cpp",
            "../imgui/imgui.cpp",
            "../imgui/imgui_demo.cpp",
            "../imgui/imgui_draw.cpp",
            "../imgui/imgui_tables.cpp",
            "../imgui/imgui_widgets.cpp",
        },
        .flags = cpp_flags.items,
    });
    kitchen.root_module.addIncludePath(b.path("../imgui"));
    kitchen.root_module.addIncludePath(b.path("../imgui/backends"));

    // install
    b.installArtifact(kitchen);

    // build step
    const build_step = b.step("kitchen", "Build kitchen");
    build_step.dependOn(&b.addInstallArtifact(kitchen, .{}).step);

    // run step
    const run_exe = b.addRunArtifact(kitchen);
    const run_step = b.step("debug", "Run kitchen");
    run_step.dependOn(&run_exe.step);
}

pub fn default_c_flags(b: *std.Build, options: BuildOptions) !std.ArrayList([]const u8) {
    var flags: std.ArrayList([]const u8) = .empty;
    try flags.appendSlice(b.allocator, &[_][]const u8{
        "-std=c23",
        "-g",
        "-Werror",
        "-Wall",
        "-Wextra",
    });
    if (options.profile) {
        try flags.append(b.allocator, "-DTRACY_ENABLE");
    }
    if (options.target.result.os.tag == .macos) {
        try flags.append(b.allocator, "-Wno-error=deprecated-declarations");
    }
    return flags;
}

pub fn default_cpp_flags(b: *std.Build, options: BuildOptions) !std.ArrayList([]const u8) {
    var flags: std.ArrayList([]const u8) = .empty;
    try flags.appendSlice(b.allocator, &[_][]const u8{
        "-std=c++11",
        "-Wall",
        "-Wformat",
    });
    if (options.profile) {
        try flags.append(b.allocator, "-DTRACY_ENABLE");
    }
    return flags;
}
