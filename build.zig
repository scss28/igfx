const std = @import("std");
const mem = std.mem;
const fs = std.fs;
const io = std.io;
const process = std.process;
const unicode = std.unicode;

const cpp_flags: []const []const u8 = &.{
    "-std=c++23",
    "-Wall",
    "-Werror",
    "-Wextra",
    "-pedantic",
    "-fno-exceptions",
    "-fno-rtti",

    "-include",
    "src/pch.h",
};

pub const AddExecutableOptions = struct {
    name: []const u8,
    module: *std.Build.Module,
};

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

    const path = b.pathJoin("shaders/{s}", .{name});
    _ = b.addInstallFile(
        compile.addOutputFileArg(b.fmt("{s}.spv", .{name})),
        path,
    );

    _ = mod;
}

pub fn build(b: *std.Build) void {
    defer _ = @import("cdb").addStep(b, "cdb");

    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const libcx = b.dependency("libcx", .{
        .target = target,
        .optimize = optimize,
    }).artifact("libcx");

    const lib_mod = b.addModule("igfx", .{
        .target = target,
        .optimize = optimize,
    });

    lib_mod.addCSourceFiles(.{
        .files = &.{
            "src/core/window.cpp",
            "src/core/graphics.cpp",

            "src/engine.cpp",
            "src/window.cpp",
            "src/graphics.cpp",
            "src/linalg.cpp",
        },
        .flags = cpp_flags,
    });

    if (optimize == .Debug) {
        lib_mod.addCMacro("DEBUG", "");
    }

    lib_mod.addIncludePath(b.path("include"));
    lib_mod.addIncludePath(b.path("src"));
    lib_mod.linkLibrary(libcx);

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

    lib_mod.addIncludePath(.{
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
    lib_mod.linkLibrary(glfw.artifact("glfw3"));

    const lib = b.addLibrary(.{
        .name = "igfx",
        .root_module = lib_mod,
    });

    b.installArtifact(lib);

    const wrapper_mod = b.addModule("igfx_wrapper", .{
        .target = target,
        .optimize = optimize,
    });

    wrapper_mod.addCSourceFiles(.{
        .files = &.{"src/main.cpp"},
        .flags = cpp_flags,
    });

    wrapper_mod.addIncludePath(b.path("include"));
    wrapper_mod.addIncludePath(b.path("src"));
    wrapper_mod.linkLibrary(lib);
    wrapper_mod.linkLibrary(libcx);

    wrapper_mod.addLibraryPath(.{
        .cwd_relative = b.pathJoin(&.{ vulkan_sdk_path, "Lib" }),
    });

    switch (target.result.os.tag) {
        .windows => {
            wrapper_mod.linkSystemLibrary("vulkan-1", .{});
        },
        else => {
            wrapper_mod.linkSystemLibrary("vulkan", .{});
        }
    }

    const user_mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
    });

    user_mod.addCSourceFile(.{
        .file = b.path("example/src/main.cpp"),
        .flags = &.{"-std=c++23"},
    });

    user_mod.addIncludePath(b.path("include"));
    user_mod.linkLibrary(lib);
    user_mod.linkLibrary(libcx);

    const user_lib = b.addLibrary(.{
        .name = "user",
        .root_module = user_mod,
        .linkage = if (optimize == .Debug) .dynamic else .static,
    });

    const exe = b.addExecutable(.{
        .name = "app",
        .root_module = wrapper_mod,
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

        wrapper_mod.addCMacro("USER_DLL", b.fmt("R\"({s})\"", .{user_install_path}));
        exe.step.dependOn(&user_install.step);
    } else {
        wrapper_mod.linkLibrary(user_lib);
    }

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());

    const run_step = b.step("run", "Run the example app");
    run_step.dependOn(&run_cmd.step);
}
