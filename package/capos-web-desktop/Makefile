# 指定软件包名称和版本
PKG_NAME:=capos-web-desktop
PKG_VERSION:=0.9

# 指定软件包依赖关系
PKG_DEPENDS:=+luci +luci-theme-bootstrap

# 指定软件包安装路径
PKG_INSTALL_DIR:=$(PKG_BUILD_DIR)/root-$(ARCH)

# 指定软件包释放文件
PKG_FILES:=files/*

# 指定软件包释放路径
PKG_FILE_PATHS:=/cwd

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/version.mk
include $(INCLUDE_DIR)/feeds.mk

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
