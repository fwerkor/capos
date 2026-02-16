include $(TOPDIR)/rules.mk

PKG_NAME:=capbox
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/capbox
  SECTION:=capos
  CATEGORY:=CapOS
  TITLE:=Capbox Scripts
  DEPENDS:=+bash +coreutils +jq
endef

define Package/capbox/description
 Deploy Capbox scripts to target filesystem.
endef

CAPBOX_DST_DIR:=/usr/lib

define Build/Compile
	# scripts only, nothing to compile
endef

define Package/capbox/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) ./files/capbox $(1)/usr/bin/capbox
endef

$(eval $(call BuildPackage,capbox))