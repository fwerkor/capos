PKG_NAME:=capos-lite
PKG_RELEASE:=$(COMMITCOUNT)
PKG_VERSION:=1
PKG_MAINTAINER:=FWERKOR Team

PKG_DEPENDS:=+capos-web-desktop


PKG_INSTALL_DIR:=$(PKG_BUILD_DIR)/root-$(ARCH)

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/version.mk
include $(INCLUDE_DIR)/feeds.mk
include $(INCLUDE_DIR)/package.mk

define Package/capos-lite
  SECTION:=col
  CATEGORY:=Collections
  TITLE:=capos-lite
endef

define Package/capos-lite/description
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

define Package/capos-lite/install
	$(INSTALL_DIR) $$(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/capos-lite $$(1)/usr/bin/
endef
