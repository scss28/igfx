const std = @import("std");
const fs = std.fs;
const mem = std.mem;
const panic = std.debug.panic;

var cdb_path: []const u8 = undefined;

pub const Options = struct {
    artifacts: []const *std.Build.Step.Compile,
};

pub fn addStep(b: *std.Build, options: Options) *std.Build.Step {
    const step = b.allocator.create(std.Build.Step) catch panic("OOM", .{});
    step.* = .init(.{
        .id = .custom,
        .name = "install compile_commands.json",
        .makeFn = installCompileCommands,
        .owner = b,
    });

    cdb_path = b.makeTempPath();
    for (options.artifacts) |artifact| {
        for (artifact.root_module.link_objects.items) |obj| switch (obj) {
            .c_source_files => |files| {
                files.flags = addCdbFlags(b.allocator, files.flags);
            },
            .c_source_file => |file| {
                file.flags = addCdbFlags(b.allocator, file.flags);
            },
            else => {},
        };

        step.dependOn(&artifact.step);
    }

    return step;
}

fn addCdbFlags(gpa: mem.Allocator, flags: []const []const u8) []const []const u8 {
    const new_flags = gpa.alloc([]const u8, flags.len + 2) catch panic("OOM", .{});
    new_flags[new_flags.len - 2] = "-gen-cdb-fragment-path";
    new_flags[new_flags.len - 1] = cdb_path;
    @memcpy(new_flags[0..flags.len], flags);

    return new_flags;
}

fn installCompileCommands(step: *std.Build.Step, _: std.Build.Step.MakeOptions) !void {
    const b = step.owner;

    var combined: std.ArrayListUnmanaged(u8) = .empty;
    try combined.append(b.allocator, '[');

    var cdb_dir = try fs.openDirAbsolute(cdb_path, .{
        .iterate = true,
    });
    defer cdb_dir.close();

    var it = cdb_dir.iterate();
    while (try it.next()) |entry| {
        if (entry.kind != .file) continue;

        const bytes = try cdb_dir.readFileAlloc(
            b.allocator,
            entry.name,
            5_000_000,
        );

        try combined.appendSlice(b.allocator, bytes[0 .. bytes.len - 1]);
    }

    if (combined.items.len > 1) {
        // Change trailing comma to ']'.
        combined.items[combined.items.len - 1] = ']';

        b.build_root.handle.writeFile(.{
            .sub_path = "compile_commands.json",
            .data = combined.items,
        }) catch |err| panic(
            "error writing compile_commands.json ({s})",
            .{@errorName(err)},
        );
    }
}
