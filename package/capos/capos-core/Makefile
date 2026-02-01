include $(TOPDIR)/rules.mk

PKG_NAME:=capos-core
PKG_VERSION:=0.1.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/capos-core
  SECTION:=capos
  CATEGORY:=CapOS
  TITLE:=CapOS Core Meta Package
  DEPENDS:=+capbox +capos-webpanel +sudo
endef

define Package/capos-core/description
 A meta package that pulls in CapOS base components.
endef

define Build/Compile
	# meta package, nothing to build
endef

define Package/capos-core/install
	# meta package, nothing to install
endef

$(eval $(call BuildPackage,capos-core))
