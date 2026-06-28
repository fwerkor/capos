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

case "$target/$subtarget" in
    x86/64|armsr/armv8|malta/le64)
        ;;
    *)
        echo "unsupported CapOS real boot CI target: $target/$subtarget" >&2
        exit 2
        ;;
esac

patch_pinned_rust_feed_for_ci

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
CONFIG_GRUB_BOOTOPTS="panic=5 argosfs.images=/dev/vda2"
CONFIG
        ;;
    malta/le64)
        cat >> .config <<'CONFIG'
CONFIG_TARGET_SERIAL="ttyS0"
CONFIG_KERNEL_CMDLINE="panic=5"
CONFIG
        ;;
esac
