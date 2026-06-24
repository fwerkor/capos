#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 2 ]]; then
    echo "usage: $0 <target> <subtarget>" >&2
    exit 2
fi

target="$1"
subtarget="$2"
bindir="bin/targets/$target/$subtarget"
artifact_dir="${CAPOS_CI_ARTIFACT_DIR:-ci-artifacts}"
boot_timeout_s="${CAPOS_CI_BOOT_TIMEOUT_S:-360}"
probe_url="${CAPOS_CI_PROBE_URL:-http://192.168.1.1:2000/cgi-bin/cap/api}"
mkdir -p "$artifact_dir"

log="$artifact_dir/qemu-${target}-${subtarget}.log"
response="$artifact_dir/probe-${target}-${subtarget}.json"
workdir="$(mktemp -d)"
qemu_pid=""
tap_wan="capwan$$"
tap_lan="caplan$$"

cleanup() {
    if [[ -n "$qemu_pid" ]] && kill -0 "$qemu_pid" 2>/dev/null; then
        kill "$qemu_pid" 2>/dev/null || true
        sleep 1
        kill -9 "$qemu_pid" 2>/dev/null || true
    fi
    sudo ip link set "$tap_wan" down 2>/dev/null || true
    sudo ip link set "$tap_lan" down 2>/dev/null || true
    sudo ip tuntap del dev "$tap_wan" mode tap 2>/dev/null || true
    sudo ip tuntap del dev "$tap_lan" mode tap 2>/dev/null || true
    rm -rf "$workdir"
}
trap cleanup EXIT

require_file() {
    local pattern="$1"
    local found
    found="$(find "$bindir" -maxdepth 1 -type f -name "$pattern" | sort | head -n 1 || true)"
    if [[ -z "$found" ]]; then
        echo "missing artifact matching $bindir/$pattern" >&2
        find "$bindir" -maxdepth 1 -type f | sort >&2 || true
        exit 1
    fi
    printf '%s\n' "$found"
}

require_any_file() {
    local found=""
    local pattern
    for pattern in "$@"; do
        found="$(find "$bindir" -maxdepth 1 -type f -name "$pattern" | sort | head -n 1 || true)"
        if [[ -n "$found" ]]; then
            printf '%s\n' "$found"
            return 0
        fi
    done
    echo "missing artifact matching any of: $*" >&2
    find "$bindir" -maxdepth 1 -type f | sort >&2 || true
    exit 1
}

prepare_image() {
    local src="$1"
    local dst="$workdir/$(basename "${src%.gz}")"
    if [[ "$src" == *.gz ]]; then
        gzip -dc "$src" > "$dst"
        printf '%s\n' "$dst"
    else
        printf '%s\n' "$src"
    fi
}

setup_taps() {
    sudo modprobe tun || true
    sudo ip tuntap add dev "$tap_wan" mode tap user "$(id -un)"
    sudo ip tuntap add dev "$tap_lan" mode tap user "$(id -un)"
    sudo ip link set "$tap_wan" up
    sudo ip addr add 192.168.1.2/24 dev "$tap_lan"
    sudo ip link set "$tap_lan" up
}

start_qemu() {
    local disk kernel rootfs firmware
    : > "$log"
    setup_taps

    case "$target/$subtarget" in
        x86/64)
            disk="$(prepare_image "$(require_file '*-combined.img.gz')")"
            qemu-system-x86_64 \
                -machine pc,accel=tcg \
                -cpu max \
                -smp 2 \
                -m 1024 \
                -nographic \
                -no-reboot \
                -drive "file=$disk,format=raw,if=virtio" \
                -netdev "tap,id=wan,ifname=$tap_wan,script=no,downscript=no" \
                -device "virtio-net-pci,netdev=wan,mac=52:54:00:12:34:10" \
                -netdev "tap,id=lan,ifname=$tap_lan,script=no,downscript=no" \
                -device "virtio-net-pci,netdev=lan,mac=52:54:00:12:34:11" \
                >"$log" 2>&1 &
            ;;
        armsr/armv8)
            disk="$(prepare_image "$(require_file '*-combined-efi.img.gz')")"
            firmware=""
            for candidate in \
                /usr/share/AAVMF/AAVMF_CODE.fd \
                /usr/share/qemu-efi-aarch64/QEMU_EFI.fd \
                /usr/share/edk2/aarch64/QEMU_EFI.fd; do
                if [[ -r "$candidate" ]]; then
                    firmware="$candidate"
                    break
                fi
            done
            if [[ -z "$firmware" ]]; then
                echo "missing AArch64 UEFI firmware" >&2
                exit 1
            fi
            qemu-system-aarch64 \
                -machine virt,accel=tcg,gic-version=max \
                -cpu cortex-a57 \
                -smp 2 \
                -m 1024 \
                -nographic \
                -no-reboot \
                -bios "$firmware" \
                -drive "file=$disk,format=raw,if=virtio" \
                -netdev "tap,id=wan,ifname=$tap_wan,script=no,downscript=no" \
                -device "virtio-net-pci,netdev=wan,mac=52:54:00:12:34:20" \
                -netdev "tap,id=lan,ifname=$tap_lan,script=no,downscript=no" \
                -device "virtio-net-pci,netdev=lan,mac=52:54:00:12:34:21" \
                >"$log" 2>&1 &
            ;;
        malta/le64)
            kernel="$(require_file '*-vmlinux-initramfs.elf')"
            rootfs="$(prepare_image "$(require_any_file '*-rootfs-argosfs.img.gz' '*-argosfs-rootfs.img.gz' '*-argosfs.img.gz')")"
            qemu-system-mips64el \
                -machine malta \
                -cpu MIPS64R2-generic \
                -smp 2 \
                -m 512 \
                -nographic \
                -no-reboot \
                -kernel "$kernel" \
                -append "argosfs.images=/dev/sda rootwait console=ttyS0,115200 panic=5" \
                -drive "file=$rootfs,format=raw" \
                -netdev "tap,id=wan,ifname=$tap_wan,script=no,downscript=no" \
                -device "pcnet,netdev=wan,mac=52:54:00:12:34:30" \
                -netdev "tap,id=lan,ifname=$tap_lan,script=no,downscript=no" \
                -device "pcnet,netdev=lan,mac=52:54:00:12:34:31" \
                >"$log" 2>&1 &
            ;;
        *)
            echo "unsupported target for QEMU boot check: $target/$subtarget" >&2
            exit 2
            ;;
    esac
    qemu_pid=$!
}

probe_service() {
    local deadline=$((SECONDS + boot_timeout_s))
    local last_status=0
    while (( SECONDS < deadline )); do
        if ! kill -0 "$qemu_pid" 2>/dev/null; then
            echo "QEMU exited before the service probe succeeded" >&2
            tail -n 240 "$log" >&2 || true
            return 1
        fi
        if curl -fsS --connect-timeout 2 --max-time 5 "$probe_url" -o "$response"; then
            if grep -q '"service"[[:space:]]*:[[:space:]]*"capos-webpanel-api"' "$response"; then
                echo "CapOS webpanel responded on $probe_url"
                return 0
            fi
            last_status=1
        else
            last_status=$?
        fi
        sleep 3
    done

    echo "timed out waiting for CapOS webpanel on $probe_url (last curl status: $last_status)" >&2
    tail -n 240 "$log" >&2 || true
    return 1
}

start_qemu
probe_service
