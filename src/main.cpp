#include "igfx/graphics.h"

#include "core/core.h"
#include "core/window.h"

#include <print>

#if _WIN32
#include <windows.h>
using DLLHandle = HMODULE;
#define LOAD_DLL(path) LoadLibraryA(path)
#define GET_SYM(handle, name) GetProcAddress(handle, name)
#define CLOSE_DLL(handle) FreeLibrary(handle)
#else
#include <dlfcn.h>
using DLLHandle = void*;
#define LOAD_DLL(path) dlopen(path, RTLD_NOW)
#define GET_SYM(handle, name) dlsym(handle, name)
#define CLOSE_DLL(handle) dlclose(handle)
#endif

#ifdef USER_DLL
using InitFn = void(*)();
using UpdateFn = void(*)();
using DrawFn = void(*)(igfx::Frame*);
struct {
    DLLHandle handle = nullptr;

    InitFn init;
    UpdateFn update;
    DrawFn draw;

    void reload() {
        if (handle != nullptr) {
            CLOSE_DLL(handle);
        }

        handle = LOAD_DLL(USER_DLL);
        if (handle == nullptr) {
            std::println(stderr, "error: failed to load dynamic library '{}'", USER_DLL);
            exit(1);
        }

        init = loadFn<InitFn>("init");
        update = loadFn<UpdateFn>("update");
        draw = loadFn<DrawFn>("draw");
    }

    template<typename Fn>
    Fn loadFn(const u8 name[]) {
        Fn fn = (Fn)GET_SYM(handle, name);
        if (fn == nullptr) {
            std::println(stderr, "error: failed to load function '{}'", name);
            exit(1);
        }

        return fn; 
    }
} userCode;

#else
extern void init();
extern void update();
extern void draw(igfx::Frame*);
#endif

int main() {
    igfx::core::init();
    defer { igfx::core::deinit(); };

#ifdef USER_DLL
    userCode.reload();
    userCode.init();
#else
    init();
#endif

    while (!igfx::core::window::shouldClose()) {
#ifdef USER_DLL
        userCode.update();
#else
        update();
#endif

        igfx::Frame frame;
#ifdef USER_DLL
        userCode.draw(&frame);
#else
        draw(&frame);
#endif
    }

#ifdef USER_DLL
    CLOSE_DLL(userCode.handle);
#endif
}
