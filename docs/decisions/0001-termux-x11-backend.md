# ADR 0001: Use SDL's X11 backend on Termux

Status: Accepted

## Context

Termux programs are compiled for Android's Bionic C library, so SDL's platform detection would normally
select its Android Activity integration. AI3 is a conventional command-line executable and has no Android
Activity. Its windows are instead displayed by the separate Termux:X11 server.

## Decision

The Termux CMake preset identifies the build as Linux, undefines `__ANDROID__`, enables SDL's X11 video
backend, and disables its Wayland and KMSDRM backends. AI3 links `libandroid-shmem`, which supplies the
shared-memory compatibility needed by the X11 stack on Android/Bionic. SDL continues to own window,
input, and GLES context management.

## Consequences

Termux builds require Termux:X11-compatible X11 libraries and `libandroid-shmem`; the bootstrap script
installs them. The resulting binary runs as a normal Termux process and needs a reachable X server through
`DISPLAY`. This configuration is intentionally isolated in the Termux preset and does not affect desktop
Linux builds.
