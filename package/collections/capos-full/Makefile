
include $(TOPDIR)/rules.mk

LUCI_TYPE:=col
LUCI_BASENAME:=capos-full
PKG_RELEASE:=$(COMMITCOUNT)
PKG_MAINTAINER:=FWERKOR Team

LUCI_TITLE:=CapOS Full
LUCI_DESCRIPTION:=CapOS Full Collection
LUCI_DEPENDS:= \
	+capos-web-desktop \
	+capos-standard \

PKG_LICENSE:=Apache-2.0

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/version.mk
include $(INCLUDE_DIR)/feeds.mk
include $(INCLUDE_DIR)/package.mk
