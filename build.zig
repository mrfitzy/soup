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
    const flags = if (target.result.os.tag == .macos)
        &[_][]const u8{
            "-std=c23",
            "-g",
            "-Werror",
            "-Wall",
            "-Wextra",
            "-Wno-error=deprecated-declarations",
        }
    else
        &[_][]const u8{
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
    const sdl_artifact = sdl.artifact("SDL3");
    soup.root_module.linkLibrary(sdl_artifact);

    if (target.result.os.tag == .macos and b.graph.host.result.os.tag != .macos) {
        if (b.graph.host.result.os.tag == .windows) {
            // MacOS SDK paths are incompatible with windows hosts
            std.log.err("error: cross-compiling for macOS from Windows is not supported", .{});
            std.process.exit(1);
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

    // run step
    const run_exe = b.addRunArtifact(soup);
    run_exe.addFileArg(b.path("data/ops.bin"));
    run_exe.addFileArg(b.path("data/cb_ops.bin"));
    run_exe.addFileArg(b.path("data/DMG_ROM.bin"));

    const run_step = b.step("run", "Run soup");
    run_step.dependOn(&run_exe.step);
}
