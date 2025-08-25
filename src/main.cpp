#include "engine.h"
#include "window.h"
#include "igfx/graphics.h"

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
using UpdateFn = void(*)(f32);
using DrawFn = void(*)(igfx::Frame*);
struct {
    DLLHandle handle = nullptr;
    struct {
        InitFn init;
        UpdateFn update;
        DrawFn draw;
    } fns;

    void reload() {
        if (handle != nullptr) {
            CLOSE_DLL(handle);
        }

        handle = LOAD_DLL(USER_DLL);
        if (handle == nullptr) {
            std::fatal("error: failed to load dynamic library '{}'", USER_DLL);
        }

        fns.init = loadFn<InitFn>("init");
        fns.update = loadFn<UpdateFn>("update");
        fns.draw = loadFn<DrawFn>("draw");
    }

    template<typename Fn>
    Fn loadFn(const u8 name[]) {
        Fn fn = (Fn)GET_SYM(handle, name);
        if (fn == nullptr) {
            std::fatal("error: failed to load function '{}'", name);
        }

        return fn; 
    }
} user;

#else
extern "C" void init();
extern "C" void update(f32);
extern "C" void draw(igfx::Frame*);
#endif

int main() {
    igfx::engine::init();
    defer { igfx::engine::deinit(); };

#ifdef USER_DLL
    user.reload();
    user.fns.init();
#else
    init();
#endif

    while (!igfx::window::shouldClose()) {
        f32 deltaTime = 1.0f;
#ifdef USER_DLL
        user.fns.update(deltaTime);
#else
        update(deltaTime);
#endif

        igfx::Frame frame;
#ifdef USER_DLL
        user.fns.draw(&frame);
#else
        draw(&frame);
#endif
    }

#ifdef USER_DLL
    CLOSE_DLL(user.handle);
#endif
}
