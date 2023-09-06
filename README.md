![OpenWrt logo](include/logo.png)

CapOS is an open source server operating system based on OpenWrt. It aims to provide an easy-to-use Linux server OS for beginners. CapOS utilizes the advantages of OpenWrt including the LuCI web-based management interface, to customize the development into a full-featured server OS.  

CapOS是一个基于OpenWrt的开源服务器操作系统。 它旨在为初学者提供易于使用的 Linux 服务器操作系统。 CapOS 利用 OpenWrt 的优势，包括 LuCI 基于 Web 的管理界面，定制开发成一个全功能的服务器操作系统。


Powered by FWERKOR Team, especially by Castronaut. 


At present, this project is still in the initial stage of development, and there is no version available for the time being. 

***目前本项目还处于开发的起始阶段，暂时没有可用版本。***


## Features

CapOS is an lightweight open source server operating system derived from OpenWrt. It is designed to provide an easy-to-use Linux server experience for beginners. CapOS makes full use of the advantages of OpenWrt such as the LuCI web interface to develop into a fully functional server OS.

Some key highlights of CapOS:

* **Highly Lightweight**: CapOS is built from OpenWrt, an embedded operating system based on Linux. It has a small footprint that can run on various hardware from routers to PCs. The lightweight design allows CapOS to run smoothly even on devices with limited resources.

* **Friendly Web UI**: CapOS provides an simple but powerful web-based interface LuCI for easy server management. Users can easily manage system settings, monitor resources and install packages through the web UI without much Linux knowledge.  

* **Highly Extensible**: Although lightweight, CapOS is highly extensible. Users can install and run many server applications and packages on demand based on their needs. CapOS aims to strike a balance between being lightweight and practical.

* **Easy to Learn**: CapOS has a clean command line interface with standardized Linux commands to simplify operation. The automated and simplified CLI allows beginners to easily get started with CapOS.

* **Stable and Secure**: CapOS is developed based on OpenWrt, a secure and stable Linux system, with the latest kernel and software packages to ensure maximum security and stability.  

* **Runs on Various Hardware**: CapOS can run on popular hardware from routers, PCs to virtual machines. The lightweight design makes it possible to operate CapOS even on resource-constrained devices.  

* **Open Source**: CapOS is released under the GPL license. The open source community is welcome to contribute to make CapOS even more lightweight, extendable and user-friendly.

CapOS aims to provide beginners with an easy-to-use yet practical Linux server OS for learning and practice. The simplified web UI and CLI makes it easy to get started with CapOS. If you are looking for a lightweight but extensible Linux server solution, try CapOS! CapOS is open and free for all. Your feedback and suggestions will help us improve CapOS.

CapOS 是从 OpenWrt 衍生出来的轻量级开源服务器操作系统。 它旨在为初学者提供易于使用的 Linux 服务器体验。 CapOS充分利用了OpenWrt的LuCI web界面等优势，开发成为一个功能齐全的服务器操作系统。

CapOS 的一些主要亮点：

* **高度轻量级**：CapOS 由基于 Linux 的嵌入式操作系统 OpenWrt 构建而成。 它占地面积小，可以在从路由器到 PC 的各种硬件上运行。 轻量级设计让 CapOS 即使在资源有限的设备上也能流畅运行。

* **友好的 Web 用户界面**：CapOS 提供了一个简单但功能强大的基于 Web 的界面 LuCI，以便于服务器管理。 用户无需太多 Linux 知识即可通过 Web UI 轻松管理系统设置、监控资源和安装软件包。

* **高度可扩展**：虽然轻量级，但 CapOS 具有高度可扩展性。 用户可以根据自己的需要，按需安装和运行许多服务器应用程序和软件包。 CapOS 旨在在轻量级和实用性之间取得平衡。

* **易于学习**：CapOS 具有简洁的命令行界面和标准化的 Linux 命令以简化操作。 自动化和简化的 CLI 让初学者可以轻松上手 CapOS。

* **稳定安全**：CapOS基于安全稳定的Linux系统OpenWrt开发，采用最新的内核和软件包，最大限度保证安全性和稳定性。

* **在各种硬件上运行**：CapOS 可以在从路由器、PC 到虚拟机的流行硬件上运行。 轻巧的设计使得即使在资源受限的设备上也能运行 CapOS。

* **开源**：CapOS 在 GPL 许可下发布。 欢迎开源社区做出贡献，使 CapOS 更加轻巧、可扩展和用户友好。

CapOS 旨在为初学者提供一个易于使用且实用的 Linux 服务器操作系统，以供学习和实践。 简化的 Web UI 和 CLI 使 CapOS 的入门变得容易。 如果您正在寻找轻量级但可扩展的 Linux 服务器解决方案，请尝试 CapOS！ CapOS 对所有人开放且免费。 您的反馈和建议将帮助我们改进 CapOS。

## Download

Generally speaking, the server of the FWERKOR team will automatically obtain the source code from Github and compile it every once in a while. The compiled version will be available for viewing and downloading in *FR-REPO*.

通常来说，FWERKOR团队的服务器会每隔一段时间自动从Github获取源代码并进行编译，编译完成后的版本将可以在*FR-REPO*查看并下载。

* [FR-REPO](https://repo.fwerkor.com/)



## 

The compiled version we provide may not include support for the user's hardware architecture, so users with relevant professional knowledge can compile CapOS by themselves. 

For everything else than simple firmware download, You can refer to some official documentation provided by OpenWrt:

* [OpenWrt Wiki Download](https://openwrt.org/downloads)

## Development

To build your own firmware you need a GNU/Linux, BSD or MacOSX system (case
sensitive filesystem required). Cygwin is unsupported because of the lack of a
case sensitive file system.

### Requirements

You need the following tools to compile CapOS, the package names vary between distributions. A complete list with distribution specific packages can be found in OpenWrt's [Build System Setup](https://openwrt.org/docs/guide-developer/build-system/install-buildsystem)
documentation.

```
binutils bzip2 diff find flex gawk gcc-6+ getopt grep install libc-dev libz-dev
make4.1+ perl python3.7+ rsync subversion unzip which
```

### Quickstart

1. Run `./scripts/feeds update -a` to obtain all the latest package definitions
   defined in feeds.conf / feeds.conf.default

2. Run `./scripts/feeds install -a` to install symlinks for all obtained
   packages into package/feeds/

3. Run `make menuconfig` to select your preferred configuration for the
   toolchain, target system & firmware packages.

4. Run `make` to build your firmware. This will download all sources, build the
   cross-compile toolchain and then cross-compile the GNU/Linux kernel & all chosen
   applications for your target system.

### Related Repositories

The main repository uses multiple sub-repositories to manage packages of
different categories. All packages are installed via the OpenWrt package
manager called `opkg`. If you're looking to develop the web interface or port
packages to OpenWrt, please find the fitting repository below.

* [CapOS](https://github.com/fwerkor/capos-packages): CapOS's official package source.

* [LuCI Web Interface](https://github.com/openwrt/luci): Modern and modular interface to control the device via a web browser.

* [OpenWrt Packages](https://github.com/openwrt/packages): Community repository of ported packages.

* [OpenWrt Routing](https://github.com/openwrt/routing): Packages specifically focused on (mesh) routing.

* [OpenWrt Video](https://github.com/openwrt/video): Packages specifically focused on display servers and clients (Xorg and Wayland).

### Documentation

* [FWERKOR Blog (For Capos)](https://blog.fwerkor.com/category/capos)

### Community

* [Github Issues](https://github.com/fwerkor/capos/Issues): For bug feedback, feature update suggestions. 
* [FWERKOR Blog](https://blog.fwerkor.com/category/capos/): You can comment below the article. 

## License

CapOS is licensed under GPL-2.0
