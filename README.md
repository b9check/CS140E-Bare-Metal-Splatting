# CS140E Final Project

Brian's code is in [`gsplat/`](gsplat/) - a minimal 3D Gaussian splatting renderer. 

I have a python implementation and a C one designed to run without dynamic memory allocation on our Pi's. The C code does steps 1-6 (see [Steps.md](gsplat/Steps.md)); scene data is embedded at build time via `xxd` (no filesystem).

See the [gsplat README](gsplat/README.md) for more info.

## Setup

**1. Add environment variable** (in `~/.bashrc`, next to `CS140E_2026_PATH`):
```
export CS140E_SPLAT_PATH=/home/brian-check/Desktop/CS140E-Bare-Metal-Splatting
```

**2. Rebuild libpi** (needed once, since we modified it):
```
cd $CS140E_SPLAT_PATH/libpi && make clean && make
```

**3. Build and run:**
```
make          # compile only
make run      # compile (if needed) and run on Pi
make clean    # remove objs/, .bin, .list
```

## Changes to libpi / class code

### Path / project setup

**`libpi/defs.mk`** — Switched from `CS140E_2026_PATH` to `CS140E_SPLAT_PATH` throughout. Defines `LPP`, `LPI`, etc. relative to this project path.

**`libpi/mk/Makefile.robust-v2`** — Uses `$(CS140E_SPLAT_PATH)/libpi/defs.mk` instead of the class repo path.

### VFP (hardware float) support

The Pi's ARM1176JZF-S has a VFP floating-point unit, but it's disabled by default. We need it for the splatting math.

**`libpi/staff-start.S`** — Enable the VFP coprocessor at boot (lines 19-25):
```asm
mrc p15, 0, r0, c1, c0, 2   @ grant access to CP10/CP11
orr r0, r0, #(0xF << 20)
mcr p15, 0, r0, c1, c0, 2
mcr p15, 0, r0, c7, c5, 4   @ instruction sync barrier (ARMv6 compatible)
mov r0, #(1 << 30)
vmsr fpexc, r0               @ turn on the FPU
```

**`libpi/defs.mk`** — Added `-Wa,-mfpu=vfp` to `CPP_ASFLAGS` so the assembler accepts VFP instructions in `.S` files.

### Build system (libpi self-build)

**`libpi/manifest.mk`** — Uses `include ./mk/Makefile.template-fixed` instead of the class Makefile template.

**`libpi/mk/Makefile.template-fixed`** — Uses `include defs.mk` (local defs) instead of `$(CS140E_2026_PATH)/libpi/defs.mk`.

### libm linking (newlib sqrtf, cosf, sinf)

Newlib's `libm.a` expects `__errno()` and pulls in `__aeabi_fcmpun` from libgcc. Bare-metal has no libc.

**`libpi/libc/errno-stub.c`** — New file. Defines `__errno()` returning a pointer to a static `_errno`. Newlib libm calls this for error handling.

**`libpi/mk/Makefile.robust-v2`** — Added `$(LDFLAGS)` to the linker command so the project can pass `-u __errno` to force pulling `errno-stub.o` from `libpi.a` before `libm.a` is processed.

### Project Makefile (top-level)

**`Makefile`** — `LIB_POST += $(LIBM) $(LIBGCC)` so libgcc comes after libm and provides `__aeabi_fcmpun` for newlib. `LDFLAGS += -u __errno` forces the linker to pull `errno-stub.o` from `libpi.a`. `RUN = 0` so `make` only compiles; use `make run` to run. `USE_SIMPLE_BUILD_DIR = 1` uses `objs/` directly instead of `objs/l1/l2/l3/` (safe since we have no `..` in paths).

**`libpi/mk/Makefile.robust-v2`** — Added `USE_SIMPLE_BUILD_DIR` option; when set to 1, skips the l1/l2/l3 subdir hack.
