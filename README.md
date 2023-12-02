![OpenWrt logo](include/logo.png)

CapOS是一个基于OpenWrt的开源服务器操作系统。 它旨在提供更适合用于家庭部署和专业部署且易于使用的 Linux 服务器操作系统。 CapOS 利用 OpenWrt 的优势，包括基于LuCI改进而成的CWD（CapOS Web Desktop）管理界面，定制开发成一个全功能的服务器操作系统。

Powered by FWERKOR Team, especially by Castronaut. 

## Features

CapOS 是从 OpenWrt 衍生出来的轻量级开源服务器操作系统。 它旨在为初学者提供易于使用的 Linux 服务器体验。 CapOS 利用 OpenWrt 的优势，包括基于LuCI改进而成的CWD（CapOS Web Desktop）管理界面，开发成为一个全功能的服务器操作系统。

CapOS 的一些主要亮点：

* **高度轻量级**：CapOS 由基于 Linux 的嵌入式操作系统 OpenWrt 构建而成。 它占地面积小，可以在从路由器到 PC 的各种硬件上运行。 轻量级设计让 CapOS 即使在资源有限的设备上也能流畅运行。

* **友好的 Web 用户界面**：CapOS 提供了一个基于LuCI改进而成的CWD（CapOS Web Desktop）管理界面，可以在浏览器中通过美观易用的界面控制服务器。 用户无需太多 Linux 知识即可通过 Web UI 轻松管理系统设置、监控资源和安装软件包等。

* **高度可扩展**：虽然轻量级，但 CapOS 具有高度可扩展性。 用户可以根据自己的需要，按需安装和运行许多服务器应用程序和软件包。 CapOS 旨在在轻量级和实用性之间取得平衡。

* **易于学习**：CapOS 具有简洁的命令行界面和标准化的 Linux 命令以简化操作。 自动化和简化的 CLI 让初学者可以轻松上手 CapOS。

* **稳定安全**：CapOS基于安全稳定的Linux系统OpenWrt开发，采用最新的内核和软件包，最大限度保证安全性和稳定性。

* **在各种硬件上运行**：CapOS 可以在从路由器、PC 到虚拟机的流行硬件上运行。 轻巧的设计使得即使在资源受限的设备上也能运行 CapOS。

* **开源**：CapOS 在 GPL 许可下发布。 欢迎开源社区做出贡献，使 CapOS 更加轻巧、可扩展和用户友好。

CapOS 旨在为初学者提供一个易于使用且实用的 Linux 服务器操作系统，以供学习和实践。 简化的 Web UI 和 CLI 使 CapOS 的入门变得容易。 如果您正在寻找轻量级但可扩展的 Linux 服务器解决方案，请尝试 CapOS！ CapOS 对所有人开放且免费。 您的反馈和建议将帮助我们改进 CapOS。

## Download

* [Github Releases](https://github.com/fwerkor/capos/releases)
* [CapOS Repository](https://capos.fwerkor.com/repository)

The compiled version we provide may not include support for the user's hardware architecture, so users with relevant professional knowledge can compile CapOS by themselves. 


## Development

To build your own firmware you need a GNU/Linux, BSD or MacOSX system (case sensitive filesystem required). Cygwin is unsupported because of the lack of a case sensitive file system.

### Requirements

You need the following tools to compile CapOS, the package names vary between distributions. A complete list with distribution specific packages can be found in OpenWrt's [Build System Setup](https://openwrt.org/docs/guide-developer/build-system/install-buildsystem)
documentation. CapOS has provided a script that can automatically install dependencies on some system, but it is unstable so do it yourself if error occurs. 

```
binutils bzip2 diff find flex gawk gcc-6+ getopt grep install libc-dev libz-dev
make4.1+ perl python3.7+ rsync subversion unzip which
```

### Build

Now, you can simply run `bash ./scripts/quickstart.sh` to start, or do the following steps yourself.

1. Run `bash ./scripts/auto_install_dependencies.sh` to automatically install dependencies on some system, It is unstable so do it yourself if error occurs. 

2. Run `./scripts/feeds update -a` to obtain all the latest package definitions defined in feeds.conf / feeds.conf.default

3. Run `./scripts/feeds install -a` to install symlinks for all obtained packages into package/feeds/

4. Run `make menuconfig` to select your preferred configuration for the toolchain, target system & firmware packages. Choosing LuCI is recommended

5. (Optional) Run `make download` to download sources required to ensure the stability of compiling. It would be helpful especially in China Mainland.

6. Run `make` to build your firmware. This will download all sources, build the cross-compile toolchain and then cross-compile the GNU/Linux kernel & all chosen applications for your target system.

## Useage

Web panel is on 2000/tcp (http) and 2020/tcp (https). 

Telnet is running on 23/tcp before root password is set. 

SSH will run on 22/tcp after root password is set. 

## About

### Related Repositories

The main repository uses multiple sub-repositories to manage packages of
different categories. All packages are installed via the CapOS package
manager called `opkg`. If you're looking to develop the web interface or port
packages to CapOS, please find the fitting repository below.

* [CapOS Web Desktop](https://github.com/fwerkor/capos-web-desktop): CapOS Web Desktop.

* [CapOS Packages](https://github.com/fwerkor/capos-packages): CapOS's official package source.

* [CapOS LuCI Web Interface](https://github.com/fwerkor/capos-luci): Modern and modular interface to control the device via a web browser.

* [CapOS Routing](https://github.com/fwerkor/capos-routing): Packages specifically focused on (mesh) routing.

* [CapOS Video](https://github.com/fwerkor/capos-video): Packages specifically focused on display servers and clients (Xorg and Wayland).

* [CapOS Telephony](https://github.com/fwerkor/capos-video): Packages community maintained for telephony.
 
* [CapOS Targets](https://github.com/fwerkor/capos-video): Packages not maintained in mainline anymore.


### Documentation

* [FWERKOR Blog (For CapOS)](https://blog.fwerkor.com/category/capos)

### Community

* [Github Issues](https://github.com/fwerkor/capos/issues): For bug feedback, feature update suggestions.
* [Github Discussions](https://github.com/fwerkor/capos/discussions): For bug feedback, feature update suggestions.

### License

CapOS is licensed under GPL-2.0. 
