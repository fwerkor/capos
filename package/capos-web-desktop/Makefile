PKG_NAME:=capos-web-desktop
PKG_RELEASE:=$(COMMITCOUNT)
PKG_VERSION:=0.9
PKG_MAINTAINER:=FWERKOR Team

PKG_DEPENDS:=+luci +luci-theme-bootstrap +luci-app-opkg
PKG_FILE_DEPENDS:=$(PLATFORM_DIR)/ $(GENERIC_PLATFORM_DIR)/capos-web-desktop/
PKG_BUILD_DEPENDS:=usign/host ucert/host

PKG_INSTALL_DIR:=$(PKG_BUILD_DIR)/root-$(ARCH)

PKG_FILES := https://raw.githubusercontent.com/fwerkor/capos-web-desktop/main/files/*
PKG_FILE_PATHS:=/cwd

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/version.mk
include $(INCLUDE_DIR)/feeds.mk
include $(INCLUDE_DIR)/package.mk

define Package/capos-web-desktop
  SECTION:=net
  CATEGORY:=Network
  TITLE:=capos-web-desktop
endef

define Package/capos-web-desktop/description
  CapOS Web Desktop Core.
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

define Package/capos-web-desktop/install
	$(INSTALL_DIR) $$(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/capos-web-desktop $$(1)/usr/bin/
endef

$(eval $(call BuildPackage,capos-web-desktop))
