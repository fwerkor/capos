![OpenWrt logo](include/logo.png)

CapOS is an open source server operating system based on OpenWrt. It is designed to provide an easy-to-use Linux server operating system that is more suitable for home and professional deployments. CapOS takes advantage of OpenWrt, including the CWD (CapOS Web Desktop) management interface improved based on LuCI, and is customized and developed into a full-featured server operating system.

Powered by FWERKOR Team, especially by Castronaut.

***Currently, this project is still in the initial stages of development and there is currently no available version. ***


## Features

CapOS is a lightweight open source server operating system derived from OpenWrt. It is designed to provide beginners with an easy-to-use Linux server experience. CapOS takes advantage of OpenWrt, including the CWD (CapOS Web Desktop) management interface improved based on LuCI, to develop into a full-featured server operating system.

Some of the key highlights of CapOS:

* **Highly lightweight**: CapOS is built on OpenWrt, a Linux-based embedded operating system. It has a small footprint and can run on a variety of hardware, from routers to PCs. The lightweight design allows CapOS to run smoothly even on devices with limited resources.

* **Friendly Web User Interface**: CapOS provides a CWD (CapOS Web Desktop) management interface improved based on LuCI, which can control the server through a beautiful and easy-to-use interface in the browser. Users can easily manage system settings, monitor resources, install software packages, etc. through the Web UI without much Linux knowledge.

* **Highly Scalable**: Although lightweight, CapOS is highly scalable. Users can install and run many server applications and software packages on-demand based on their needs. CapOS aims to strike a balance between lightweight and practicality.

* **Easy to Learn**: CapOS features a clean command line interface and standardized Linux commands to simplify operations. Automation and a simplified CLI make it easy for beginners to get started with CapOS.

* **Stable and secure**: CapOS is developed based on the safe and stable Linux system OpenWrt, using the latest kernel and software packages to maximize security and stability.

* **Run on a variety of hardware**: CapOS can run on popular hardware from routers and PCs to virtual machines. The lightweight design enables CapOS to run even on resource-constrained devices.

* **Open Source**: CapOS is released under the GPL license. The open source community is welcome to contribute to make CapOS more lightweight, scalable and user-friendly.

CapOS is designed to provide beginners with an easy-to-use and practical Linux server operating system to learn and practice. A simplified web UI and CLI make getting started with CapOS easy. If you're looking for a lightweight yet scalable Linux server solution, try CapOS! CapOS is open and free to everyone. Your feedback and suggestions will help us improve CapOS.

## Download

* [Github Releases](https://github.com/fwerkor/capos/releases)
* [CapOS Repository](https://capos.fwerkor.com/repository)

The compiled version we provide may not include support for the user's hardware architecture, so users with relevant professional knowledge can compile CapOS by themselves.


## Development

To build your own firmware you need a GNU/Linux, BSD or MacOSX system (case sensitive filesystem required). Cygwin is unsupported because of the lack of a case sensitive file system.

### Requirements

You need the following tools to compile CapOS, the package names vary between distributions. A complete list with distribution specific packages can be found in OpenWrt's [Build System Setup](https://openwrt.org/docs/guide-developer/build- system/install-buildsystem)
documentation. We CapOS has provided a script that can automatically install dependencies on some system, but it is unstable so do it yourself if error occurs.

```
binutils bzip2 diff find flex gawk gcc-6+ getopt grep install libc-dev libz-dev
make4.1+ perl python3.7+ rsync subversion unzip which
```

### Quickstart

1. Run `bash ./scripts/auto_install_dependencies.sh` to automatically install dependencies on some system, It is unstable so do it yourself if error occurs.

2. Run `./scripts/feeds update -a` to obtain all the latest package definitions defined in feeds.conf / feeds.conf.default

3. Run `./scripts/feeds install -a` to install symlinks for all obtained packages into package/feeds/

4. Run `make menuconfig` to select your preferred configuration for the toolchain, target system & firmware packages.

5. (Optional) Run `make download` to download sources required to ensure the stability of compiling. It would be helpful especially in China Mainland.

6. Run `make` to build your firmware. This will download all sources, build the cross-compile toolchain and then cross-compile the GNU/Linux kernel & all chosen applications for your target system.

### Related Repositories

The main repository uses multiple sub-repositories to manage packages of
different categories. All packages are installed via the CapOS package
manager called `opkg`. If you're looking to develop the web interface or port
packages to CapOS, please find the fitting repository below.

* [CapOS Packages](https://github.com/fwerkor/capos-packages): CapOS's official package source.

* [CapOS LuCI Web Interface](https://github.com/fwerkor/capos-luci): Modern and modular interface to control the device via a web browser.

* [CapOS Routing](https://github.com/fwerkor/capos-routing): Packages specifically focused on (mesh) routing.

* [CapOS Video](https://github.com/fwerkor/capos-video): Packages specifically focused on display servers and clients (Xorg and Wayland).

* [CapOS Telephony](https://github.com/fwerkor/capos-video): Packages community maintained for telephony.
 
* [CapOS Targets](https://github.com/fwerkor/capos-video): Packages not maintained in mainline anymore.

### Documentation

* [FWERKOR Blog (For Capos)](https://blog.fwerkor.com/category/capos)

### Community

* [Github Issues](https://github.com/fwerkor/capos/issues): For bug feedback, feature update suggestions.
* [Github Discussions](https://github.com/fwerkor/capos/discussions): For bug feedback, feature update suggestions.

## License

This software is allowed to be used free of charge for non-profit purposes. It cannot be used for commercial purposes without the permission of the developer.
