# AI3

AI3 is a C++ project targeting OpenGL ES 3 through SDL3, with Dear ImGui (docking branch) for the user interface.

The initial supported build targets are:

- Termux on ARM64 using Clang.
- Desktop Linux on x86-64 using GCC.

Configure, build, and test with the preset for your platform:

```sh
bash scripts/check.sh
```

Run `../build/AI3/termux-clang-debug/ai3` on Termux or
`../build/AI3/linux-gcc-debug/ai3` on desktop Linux. The application creates an
SDL3-managed OpenGL ES 3 context and provides a dockspace, a Hello World window,
and menu access to Dear ImGui demo and diagnostic windows.

The Termux preset identifies the target as Linux for CMake platform detection.
Termux's compiler still targets Android's Bionic environment, but SDL must select
its Unix/X11 backend rather than its Android application backend when AI3 runs
through Termux:X11. The preset suppresses SDL's Android Activity conditionals,
links Termux's shared-memory compatibility library, and disables non-X11 Unix
video backends. The Termux build preset is limited to two parallel jobs. SDL is
linked statically as an application-owned FetchContent dependency.
