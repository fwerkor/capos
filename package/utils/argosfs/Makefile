include $(TOPDIR)/rules.mk

PKG_NAME:=argosfs
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/fwerkor/argosfs
PKG_SOURCE_VERSION:=main
PKG_MIRROR_HASH:=skip

PKG_MAINTAINER:=FWERKOR
PKG_LICENSE:=Apache-2.0
PKG_LICENSE_FILES:=LICENSE

PKG_BUILD_DEPENDS:=rust/host fuse3 argosfs/host
HOST_BUILD_DEPENDS:=rust/host
PKG_BUILD_PARALLEL:=1
HOST_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/host-build.mk
include $(INCLUDE_DIR)/package.mk
include $(TOPDIR)/feeds/packages/lang/rust/rust-package.mk
include $(TOPDIR)/feeds/packages/lang/rust/rust-host-build.mk

HOST_FUSE3_PKG_CONFIG_LIBDIR:=$(shell env -u PKG_CONFIG_LIBDIR -u PKG_CONFIG_PATH PATH=/usr/local/bin:/usr/bin:/bin pkg-config --variable pc_path pkg-config 2>/dev/null)
CARGO_HOST_VARS += PKG_CONFIG_LIBDIR=$(STAGING_DIR_HOSTPKG)/lib/pkgconfig:$(STAGING_DIR_HOST)/lib/pkgconfig:$(HOST_FUSE3_PKG_CONFIG_LIBDIR)

define Package/argosfs
  SECTION:=utils
  CATEGORY:=Utilities
  SUBMENU:=Filesystem
  TITLE:=ArgosFS root filesystem daemon and tools
  URL:=https://github.com/fwerkor/argosfs
  DEPENDS:=$(RUST_ARCH_DEPENDS) +libfuse3 +fuse3-utils +kmod-fuse +libpthread
endef

define Package/argosfs/description
 ArgosFS userspace filesystem, raw/loop block backend tooling, and CapOS
 initramfs rootfs integration.
endef

define Package/argosfs/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/bin/argosfs $(1)/usr/sbin/argosfs
	$(INSTALL_DIR) $(1)/lib/argosfs
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/contrib/capos/initramfs/argosfs-root.sh $(1)/lib/argosfs/argosfs-root.sh
	$(INSTALL_DIR) $(1)/lib/argosfs/initramfs-hooks
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/contrib/capos/initramfs/hooks/argosfs $(1)/lib/argosfs/initramfs-hooks/argosfs
	$(INSTALL_DIR) $(1)/lib/preinit
	$(INSTALL_BIN) ./files/05_argosfs_root $(1)/lib/preinit/05_argosfs_root
	$(INSTALL_BIN) ./files/79_argosfs_skip_mount_root $(1)/lib/preinit/79_argosfs_skip_mount_root
	$(INSTALL_BIN) ./files/81_argosfs_restore_preinit $(1)/lib/preinit/81_argosfs_restore_preinit
	$(INSTALL_DIR) $(1)/etc/argosfs
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/contrib/capos/mkinitramfs/argosfs.conf $(1)/etc/argosfs/initramfs.conf
	$(INSTALL_DIR) $(1)/lib/systemd/system
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/contrib/capos/systemd/argosfs-root.service $(1)/lib/systemd/system/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/contrib/capos/systemd/argosfs-health.service $(1)/lib/systemd/system/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/contrib/capos/systemd/argosfs-watchdog.service $(1)/lib/systemd/system/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/contrib/capos/systemd/argosfs-recovery.target $(1)/lib/systemd/system/
endef

$(eval $(call RustBinHostBuild))
$(eval $(call HostBuild))
$(eval $(call BuildPackage,argosfs))
