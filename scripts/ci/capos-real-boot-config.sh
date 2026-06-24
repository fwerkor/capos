#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 2 ]]; then
    echo "usage: $0 <target> <subtarget>" >&2
    exit 2
fi

target="$1"
subtarget="$2"

patch_pinned_rust_feed_for_ci() {
    local rust_values="feeds/packages/lang/rust/rust-values.mk"

    if [[ "$target/$subtarget" != "malta/le64" || ! -f "$rust_values" ]]; then
        return 0
    fi

    if grep -q 'mips64el-unknown-linux-muslabi64' "$rust_values"; then
        return 0
    fi

    python3 - <<'PY'
from pathlib import Path

path = Path("feeds/packages/lang/rust/rust-values.mk")
text = path.read_text()
old = """else ifeq ($(ARCH),riscv64)\n  RUSTC_TARGET_ARCH:=$(subst riscv64,riscv64gc,$(RUSTC_TARGET_ARCH))\nendif\n"""
new = """else ifeq ($(ARCH),riscv64)\n  RUSTC_TARGET_ARCH:=$(subst riscv64,riscv64gc,$(RUSTC_TARGET_ARCH))\nelse ifeq ($(ARCH),mips64el)\n  RUSTC_TARGET_ARCH:=$(subst linux-musl,linux-muslabi64,$(RUSTC_TARGET_ARCH))\nendif\n"""
if old not in text:
    raise SystemExit("could not patch pinned rust feed target mapping")
path.write_text(text.replace(old, new, 1))
PY
}


patch_pinned_gpgme_feed_for_ci() {
    local patch_dir="feeds/packages/libs/gpgme/patches"
    local patch_file="$patch_dir/900-capos-ci-skip-gpgme-tool.patch"

    if [[ ! -f "feeds/packages/libs/gpgme/Makefile" ]]; then
        return 0
    fi

    mkdir -p "$patch_dir"
    if [[ -f "$patch_file" ]]; then
        return 0
    fi

    cat > "$patch_file" <<'PATCH'
--- a/src/Makefile.am
+++ b/src/Makefile.am
@@ -35,7 +35,7 @@ m4datadir = $(datadir)/aclocal
 m4data_DATA = gpgme.m4
 nodist_include_HEADERS = gpgme.h
 
-bin_PROGRAMS = gpgme-tool gpgme-json
+bin_PROGRAMS = gpgme-json
 
 if BUILD_W32_GLIB
 ltlib_gpgme_glib = libgpgme-glib.la
--- a/src/Makefile.in
+++ b/src/Makefile.in
@@ -111,7 +111,7 @@ POST_UNINSTALL = :
 build_triplet = @build@
 host_triplet = @host@
-bin_PROGRAMS = gpgme-tool$(EXEEXT) gpgme-json$(EXEEXT)
+bin_PROGRAMS = gpgme-json$(EXEEXT)
 @HAVE_W32_SYSTEM_TRUE@libexec_PROGRAMS = gpgme-w32spawn$(EXEEXT)
 subdir = src
 ACLOCAL_M4 = $(top_srcdir)/aclocal.m4
PATCH
}

case "$target/$subtarget" in
    x86/64|armsr/armv8|malta/le64)
        ;;
    *)
        echo "unsupported CapOS real boot CI target: $target/$subtarget" >&2
        exit 2
        ;;
esac

patch_pinned_rust_feed_for_ci
patch_pinned_gpgme_feed_for_ci

cat > .config <<CONFIG
CONFIG_TARGET_${target}=y
CONFIG_TARGET_${target}_${subtarget}=y
CONFIG_PACKAGE_capos-core=y
CONFIG_TARGET_ROOTFS_ARGOSFS=y
CONFIG_TARGET_KERNEL_PARTSIZE=64
CONFIG_TARGET_ROOTFS_PARTSIZE=512
CONFIG_TARGET_IMAGES_GZIP=y
CONFIG_BUILD_LOG=y
CONFIG_CCACHE=y
CONFIG_DOWNLOAD_FOLDER="dl"
CONFIG_GRUB_SERIAL=y
CONFIG_GRUB_BAUDRATE=115200
CONFIG_GRUB_TIMEOUT="1"
CONFIG

case "$target/$subtarget" in
    x86/64)
        cat >> .config <<'CONFIG'
CONFIG_GRUB_IMAGES=y
# CONFIG_GRUB_EFI_IMAGES is not set
# CONFIG_ISO_IMAGES is not set
# CONFIG_VDI_IMAGES is not set
# CONFIG_VMDK_IMAGES is not set
# CONFIG_VHDX_IMAGES is not set
CONFIG_TARGET_SERIAL="ttyS0"
CONFIG_GRUB_BOOTOPTS="panic=5"
CONFIG
        ;;
    armsr/armv8)
        cat >> .config <<'CONFIG'
CONFIG_GRUB_EFI_IMAGES=y
# CONFIG_VMDK_IMAGES is not set
CONFIG_TARGET_SERIAL="ttyAMA0"
CONFIG_GRUB_BOOTOPTS="panic=5"
CONFIG
        ;;
    malta/le64)
        cat >> .config <<'CONFIG'
CONFIG_TARGET_SERIAL="ttyS0"
CONFIG_KERNEL_CMDLINE="panic=5"
CONFIG
        ;;
esac
