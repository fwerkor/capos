PKG_NAME:=capos-full
PKG_RELEASE:=$(COMMITCOUNT)
PKG_VERSION:=1
PKG_MAINTAINER:=FWERKOR Team

PKG_DEPENDS:=+capos-standard


PKG_INSTALL_DIR:=$(PKG_BUILD_DIR)/root-$(ARCH)

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/version.mk
include $(INCLUDE_DIR)/feeds.mk
include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=col
  CATEGORY:=Collections
  TITLE:=$(PKG_NAME)
endef

define Package/$(PKG_NAME)/description
  CapOS Full Collection.
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

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $$(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $$(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
