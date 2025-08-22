# igfx
**igfx** is a graphics engine written in C++ and Vulkan.

The usual boilerplate for an igfx application looks something like this:
```C++
#include <igfx/window.h>
#include <igfx/graphics.h>

using igfx::vec2;

// Load assets, initialize application state etc.
extern void init() {
    /// ...
}

// Update application state.
extern void update(f32 deltaTime) {
    /// ...
}

// Draw the application.
extern void draw(igfx::Frame* frame) {
    /// ...
}
```
Of course these functions could be split into separate files for organization as long as they have the same signature.

## Building the example
To build the example you first need:
- [Zig](https://ziglang.org/download/) (0.15.1)
- [VulkanSDK](https://www.lunarg.com/vulkan-sdk/)
Once you have the dependencies installed, run:

- `zig build` to build
- `zig build run` to build and run

## Getting started
#### build.zig
First check if you have the prerequisites mentioned in **Building the example**.

Then create a new directory and make a new `build.zig` file and paste in the code below:
```
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
```
Run `zig fetch --save git+https://github.com/scss28/igfx` inside the project directory to pull the igfx dependency.

You can now add a `src/main.cpp` and paste in the igfx boilerplate from the top of the README.

Then to build simply run `zig build` and to run use `zig build run`.
#### CMake 
*currently unsupported...*
