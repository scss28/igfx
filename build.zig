const std = @import("std");
const fs = std.fs;
const io = std.io;

pub const AddExecutableOptions = struct {
    name: []const u8,
    module: *std.Build.Module,
};

const base_cpp_flags: []const []const u8 = &.{"-std=c++23"};
var igfx_core_lib: *std.Build.Step.Compile = undefined;

pub fn addExecutable(
    b: *std.Build,
    options: AddExecutableOptions,
) *std.Build.Step.Compile {
    const user_code_mod = options.module;
    const target = user_code_mod.resolved_target.?;
    const optimize = user_code_mod.optimize.?;

    const igfx_core_mod = b.addModule("igfx_core", .{
        .target = target,
        .optimize = optimize,
        .link_libcpp = true,
    });

    igfx_core_mod.addCSourceFiles(.{
        .files = &.{
            "src/core/window.cpp",
            "src/core/graphics.cpp",
        },
        .flags = base_cpp_flags ++ .{
            "-include",
            "src/pch.h",
        },
    });

    igfx_core_mod.addIncludePath(b.path("include"));
    igfx_core_mod.addIncludePath(b.path("src"));

    const vulkan = b.dependency("vulkan", .{});
    igfx_core_mod.addIncludePath(vulkan.path("include"));

    const glfw = b.dependency("glfw", .{});
    igfx_core_mod.addIncludePath(glfw.path("glfw/include"));
    igfx_core_mod.linkLibrary(glfw.artifact("glfw"));

    igfx_core_lib = b.addLibrary(.{
        .name = "igfx_core",
        .root_module = igfx_core_mod,
    });

    const igfx_lib_mod = b.addModule("igfx_lib", .{
        .target = target,
        .optimize = optimize,
        .link_libcpp = true,
    });

    igfx_lib_mod.addCSourceFiles(.{
        .files = &.{
            "src/window.cpp",
            "src/pch.cpp",
            "src/linalg.cpp",
        },
        .flags = base_cpp_flags ++ .{
            "-include",
            "src/pch.h",
        },
    });
    igfx_lib_mod.addIncludePath(b.path("include"));
    igfx_lib_mod.addIncludePath(b.path("src"));
    igfx_lib_mod.linkLibrary(igfx_core_lib);

    const igfx_lib = b.addLibrary(.{
        .name = "igfx",
        .root_module = igfx_lib_mod,
    });

    const igfx_exe_mod = b.addModule("igfx_exe", .{
        .target = target,
        .optimize = optimize,
        .link_libcpp = true,
    });

    igfx_exe_mod.addCSourceFiles(.{
        .files = &.{"src/main.cpp"},
        .flags = base_cpp_flags ++ .{
            "-include",
            "src/pch.h",
        },
    });

    igfx_exe_mod.addIncludePath(b.path("include"));
    igfx_exe_mod.addIncludePath(b.path("src"));

    igfx_exe_mod.linkLibrary(igfx_lib);
    igfx_exe_mod.linkLibrary(igfx_core_lib);

    user_code_mod.addIncludePath(b.path("include"));
    user_code_mod.link_libcpp = true;
    user_code_mod.linkLibrary(igfx_lib);

    const user_code_lib = b.addLibrary(.{
        .name = "user",
        .root_module = user_code_mod,
        .linkage = if (optimize == .Debug) .dynamic else .static,
    });

    const exe = b.addExecutable(.{
        .name = options.name,
        .root_module = igfx_exe_mod,
    });

    // Depending on the optimize mode either link statically or
    // provide a path to the dll enabling hot reload during debugging (TODO).
    if (optimize == .Debug) {
        const user_install = b.addInstallArtifact(user_code_lib, .{});
        const user_install_path = b.pathJoin(&.{
            b.install_path,
            "bin",
            user_install.dest_sub_path,
        });

        igfx_exe_mod.addCMacro("USER_DLL", b.fmt("R\"({s})\"", .{user_install_path}));
        exe.step.dependOn(&user_install.step);
    } else {
        igfx_exe_mod.linkLibrary(user_code_lib);
    }

    return exe;
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const example_exe_mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
    });

    example_exe_mod.addCSourceFile(.{
        .file = b.path("example/src/main.cpp"),
        .flags = &.{"-std=c++23"},
    });

    const example_exe = addExecutable(b, .{
        .name = "example",
        .module = example_exe_mod,
    });

    const run_cmd = b.addRunArtifact(example_exe);
    run_cmd.step.dependOn(b.getInstallStep());

    const run_step = b.step("run", "Run the example app");
    run_step.dependOn(&run_cmd.step);

    const cdb = @import("build/cdb.zig");
    const cdb_step = b.step("cdb", "Create compile_commands.json");
    cdb_step.dependOn(cdb.addStep(b, .{ .artifacts = &.{igfx_core_lib, example_exe} }));
}
