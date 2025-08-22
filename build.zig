const std = @import("std");
const mem = std.mem;
const fs = std.fs;
const io = std.io;
const process = std.process;
const unicode = std.unicode;

const base_cpp_flags: []const []const u8 = &.{ "-std=c++23", "-Wall" };

pub const AddExecutableOptions = struct {
    name: []const u8,
    module: *std.Build.Module,
};

pub fn addExecutable(b: *std.Build, options: AddExecutableOptions) *std.Build.Step.Compile {
    const user_mod = options.module;
    const target = user_mod.resolved_target.?;
    const optimize = user_mod.optimize.?;

    const igfx = b.dependency("igfx", .{ .target = target, .optimize = optimize });
    const igfx_lib = igfx.artifact("igfx");
    const igfx_wrapper_mod = igfx.module("igfx_wrapper");

    return addExecutableSelf(b, options.name, user_mod, igfx_lib, igfx_wrapper_mod);
}

fn addExecutableSelf(
    b: *std.Build,
    name: []const u8,
    user_mod: *std.Build.Module,
    igfx_lib: *std.Build.Step.Compile,
    igfx_wrapper_mod: *std.Build.Module,
) *std.Build.Step.Compile {
    const optimize = user_mod.optimize.?;

    user_mod.addIncludePath(b.path("include"));
    user_mod.linkLibrary(igfx_lib);

    const user_lib = b.addLibrary(.{
        .name = "user",
        .root_module = user_mod,
        .linkage = if (optimize == .Debug) .dynamic else .static,
    });

    const exe = b.addExecutable(.{
        .name = name,
        .root_module = igfx_wrapper_mod,
    });

    // Depending on the optimize mode either link statically or
    // provide a path to the dll enabling hot reload during debugging (TODO).
    if (optimize == .Debug) {
        const user_install = b.addInstallArtifact(user_lib, .{});
        const user_install_path = b.pathJoin(&.{
            b.install_path,
            "bin",
            user_install.dest_sub_path,
        });

        igfx_wrapper_mod.addCMacro("USER_DLL", b.fmt("R\"({s})\"", .{user_install_path}));
        exe.step.dependOn(&user_install.step);
    } else {
        igfx_wrapper_mod.linkLibrary(user_lib);
    }

    return exe;
}

fn embedShaderCode(
    b: *std.Build,
    vulkan_sdk_path: []const u8,
    mod: *std.Build.Module,
    src: std.Build.LazyPath,
    name: []const u8,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
) void {
    const compiler_path = b.pathJoin(&.{
        vulkan_sdk_path,
        "Bin",
        if (target.result.os.tag == .windows) "glslc.exe" else "glslc",
    });

    const compile = b.addSystemCommand(&.{compiler_path});
    compile.addArgs(&.{
        "--target-env=vulkan1.3",
    });
    switch (optimize) {
        .ReleaseFast => compile.addArgs(&.{"-O"}),
        else => {},
    }

    compile.addFileArg(src);

    switch (target.result.os.tag) {
        .windows => {
            const file = compile.addPrefixedOutputFileArg("-o", b.fmt("{s}.rc", .{name}));
            mod.addWin32ResourceFile(.{ .file = file });
        },
        else => {
            @panic("TODO");
        },
    }
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const igfx_lib_mod = b.addModule("igfx", .{
        .target = target,
        .optimize = optimize,
        .link_libcpp = true,
    });

    igfx_lib_mod.addCSourceFiles(.{
        .files = &.{
            "src/core/window.cpp",
            "src/core/graphics.cpp",

            "src/engine.cpp",
            "src/window.cpp",
            "src/graphics.cpp",
            "src/pch.cpp",
            "src/linalg.cpp",
        },
        .flags = base_cpp_flags ++ .{
            "-include",
            "src/pch.h",
        },
    });

    if (optimize == .Debug) {
        igfx_lib_mod.addCMacro("DEBUG", "");
    }

    igfx_lib_mod.addIncludePath(b.path("include"));
    igfx_lib_mod.addIncludePath(b.path("src"));

    const vulkan_sdk_path = b.option(
        []const u8,
        "vulkan",
        "Specify the Vulkan SDK path",
    ) orelse blk: {
        const env_map = process.getEnvMap(b.allocator) catch @panic("OOM");
        break :blk env_map.get("VULKAN_SDK") orelse {
            @panic("VULKAN_SDK env var not found (set the path through options)");
        };
    };

    igfx_lib_mod.addIncludePath(.{
        .cwd_relative = b.pathJoin(&.{ vulkan_sdk_path, "Include" }),
    });

    // embedShaderCode(
    //     b,
    //     vulkan_sdk_path,
    //     igfx_lib_mod,
    //     b.path("shaders/quad.frag"),
    //     "quad.frag",
    //     target,
    //     optimize,
    // );

    const glfw = b.dependency("glfw", .{ .target = target, .optimize = optimize });
    igfx_lib_mod.linkLibrary(glfw.artifact("glfw3"));

    const igfx_lib = b.addLibrary(.{
        .name = "igfx",
        .root_module = igfx_lib_mod,
    });

    b.installArtifact(igfx_lib);

    const igfx_wrapper_mod = b.addModule("igfx_wrapper", .{
        .target = target,
        .optimize = optimize,
        .link_libcpp = true,
    });

    igfx_wrapper_mod.addCSourceFiles(.{
        .files = &.{"src/main.cpp"},
        .flags = base_cpp_flags ++ .{
            "-include",
            "src/pch.h",
        },
    });

    igfx_wrapper_mod.addIncludePath(b.path("include"));
    igfx_wrapper_mod.addIncludePath(b.path("src"));
    igfx_wrapper_mod.linkLibrary(igfx_lib);

    igfx_wrapper_mod.addLibraryPath(.{
        .cwd_relative = b.pathJoin(&.{ vulkan_sdk_path, "Lib" }),
    });

    switch (target.result.os.tag) {
        .windows => {
            igfx_wrapper_mod.linkSystemLibrary("vulkan-1", .{});
        },
        else => {
            igfx_wrapper_mod.linkSystemLibrary("vulkan", .{});
        }
    }

    const example_exe_mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
    });

    example_exe_mod.addCSourceFile(.{
        .file = b.path("example/src/main.cpp"),
        .flags = &.{"-std=c++23"},
    });

    const example_exe = addExecutableSelf(
        b,
        "example",
        example_exe_mod,
        igfx_lib,
        igfx_wrapper_mod,
    );

    b.installArtifact(example_exe);

    const run_cmd = b.addRunArtifact(example_exe);
    run_cmd.step.dependOn(b.getInstallStep());

    const run_step = b.step("run", "Run the example app");
    run_step.dependOn(&run_cmd.step);

    const cdb = @import("build/cdb.zig");
    const cdb_step = b.step("cdb", "Create compile_commands.json");
    cdb_step.dependOn(cdb.addStep(b, .{ .artifacts = &.{ igfx_lib, example_exe } }));
}
