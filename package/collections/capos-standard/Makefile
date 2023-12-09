PKG_NAME:=capos-standard
PKG_RELEASE:=$(COMMITCOUNT)
PKG_VERSION:=1
PKG_MAINTAINER:=FWERKOR Team

PKG_DEPENDS:=+capos-lite kmod-usb-storage usbutils block-mount gdisk e2fsprogs kmod-fs-ext4 kmod-fs-fat32 kmod-fs-ntfs


PKG_INSTALL_DIR:=$(PKG_BUILD_DIR)/root-$(ARCH)

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/version.mk
include $(INCLUDE_DIR)/feeds.mk
include $(INCLUDE_DIR)/package.mk

define Package/capos-standard
  SECTION:=col
  CATEGORY:=Collections
  TITLE:=capos-standard
endef

define Package/capos-standard/description
  CapOS Standard Collection.
endef

define Build/Prepare
	$(call Build/Prepare/Default)
endef

define Build/Configure
	$(call Build/Configure/Default)
endef

define Build/Compile
	$(call Build/Compile/Default)
endef

define Package/capos-standard/install
	$(INSTALL_DIR) $$(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/capos-standard $$(1)/usr/bin/
endef

$(eval $(call BuildPackage,capos-standard))
