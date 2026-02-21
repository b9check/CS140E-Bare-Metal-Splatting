PROGS := gsplat/c/main.c
COMMON_SRC := gsplat/c/Preprocess.c gsplat/c/matrix-helpers.c gsplat/c/preprocess-helpers.c mbox.c

# add gsplat/c to include path so headers are found
INC += -Igsplat/c

# Embed scene.gsplat as C array so load_gaussians can read it at runtime without fopen.
# Create scene.gsplat via: python gsplat/python/ply_to_gsplat.py input.ply gsplat/scene.gsplat
gsplat/c/scene-data.h: gsplat/scene.gsplat
	xxd -i gsplat/scene.gsplat | sed 's/unsigned char/static const unsigned char/' | sed 's/unsigned int/static const unsigned int/' > $@

BOOTLOADER = my-install
RUN = 0

LIBGCC = $(shell $(CC) -print-libgcc-file-name)
LIBM = $(shell $(CC) -print-file-name=libm.a)
LIB_POST += $(LIBM) $(LIBGCC)
# Force linker to pull __errno from libpi.a (needed by newlib libm)
LDFLAGS += -u __errno

# Simpler objs layout (no objs/l1/l2/l3); safe since we have no ".." in paths
USE_SIMPLE_BUILD_DIR = 1

include $(CS140E_SPLAT_PATH)/libpi/mk/Makefile.robust-v2

gsplat/c/main.bin: gsplat/c/scene-data.h
