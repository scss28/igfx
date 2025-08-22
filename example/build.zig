const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe_mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
    });

    exe_mod.addCSourceFile(.{
        .file = b.path("src/main.cpp"),
        .flags = &.{"-std=c++23"},
    });

    const igfx = @import("igfx");
    const exe = igfx.addExecutable(b, .{
        .name = "igfx-example",
        .module = exe_mod,
    });

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);
}
