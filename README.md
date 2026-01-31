# FWERKOR CapOS

![OpenWrt logo](include/logo.png)

CapOS is an open-source server operating system based on OpenWrt. It aims to provide an easy-to-use Linux server operating system that is more suitable for both home and professional deployments. We leverage the advantages of OpenWrt and incorporate a Webdesktop management interface, customizing CapOS into a full-featured server operating system.

Maintained by FWERKOR Team, especially by Castronaut. 

## Features

* **Highly Lightweight:** CapOS is built on OpenWrt, a Linux-based embedded operating system. It has a small footprint and can run on a variety of hardware, from routers to PCs. This lightweight design allows CapOS to run smoothly even on resource-constrained devices.

* **Friendly Web User Interface:** CapOS provides a Webdesktop management interface, allowing you to control the server through a beautiful and easy-to-use interface in your browser. Users can easily manage system settings, monitor resources, and install software packages via the Web UI without requiring extensive Linux knowledge.

* **Highly Scalable:** Although lightweight, CapOS is highly scalable. Users can install and run many server applications and software packages as needed, according to their requirements. CapOS aims to strike a balance between lightweight design and practicality.

* **Easy to Learn:** CapOS features a simple command-line interface and standardized Linux commands to simplify operations. The automated and simplified CLI makes it easy for beginners to get started with CapOS.

* **Stable and Secure:** CapOS is developed based on the secure and stable Linux system OpenWrt, utilizing the latest kernel and software packages to maximize security and stability.

* **Runs on various hardware:** CapOS can run on popular hardware ranging from routers and PCs to virtual machines. Its lightweight design allows it to run even on resource-constrained devices.

* **Open Source:** CapOS is released under the GPL license. Contributions from the open-source community are welcome to make CapOS even more lightweight, scalable, and user-friendly.

CapOS aims to provide an easy-to-use and practical Linux server operating system for learning and experimentation. The simplified web UI and CLI make getting started with CapOS easy. If you are looking for a lightweight yet scalable Linux server solution, try CapOS! The CapOS Community Edition is open and free for everyone. Your feedback and suggestions will help us improve CapOS.

## Download

* [Github Releases](https://github.com/fwerkor/capos/releases)
* [FWERKOR Repository](https://repo.fwerkor.com/capos)

The compiled version we provide may not include support for the user's hardware architecture, so users with relevant professional knowledge can compile CapOS by themselves. 


## Development

As an open-source project, CapOS encourages professional developers to contribute to its improvement and to develop applications for it.

### Compile CapOS

To build your own firmware you need a GNU/Linux, BSD or MacOSX system (case sensitive filesystem required). Cygwin is unsupported because of the lack of a case sensitive file system.

You need the following tools to compile CapOS, the package names vary between distributions. A complete list with distribution specific packages can be found in OpenWrt's [Build System Setup](https://openwrt.org/docs/guide-developer/build-system/install-buildsystem) documentation. CapOS has provided a script that can automatically install dependencies on some system, but it is unstable so do it yourself if error occurs. 

```
binutils bzip2 diff find flex gawk gcc-6+ getopt grep install libc-dev libz-dev
make4.1+ perl python3.7+ rsync subversion unzip which
```

Now, you can simply follow these steps to build CapOS.

Note that do not use root user.

1. Run `bash ./scripts/auto_install_dependencies.sh` to install dependencies on your system.

2. Run `make menuconfig` to select your preferred configuration for the toolchain, target system & firmware packages. 

3. Run `make` to build your firmware. This will download all sources, build the cross-compile toolchain and then cross-compile the GNU/Linux kernel & all chosen applications for your target system.

### Develop Apps

We are committed to simplifying the CapOS application development process as much as possible. You can read our [CAPP Development Guide](https://blog.fwerkor.com/archives/1123)

We have provided some simple examples to help you quickly get started with CapOS application development.

* [HelloWorld](https://github.com/fwerkor/capp-helloworld): A minimal example of building a web service from scratch using the Go programming language.

## Useage

Web panel is on `2000/tcp` (http) and `2020/tcp` (https). 

Telnet is running on `23/tcp` before root password is set, while SSH will run on `22/tcp` after root password is set. 

Unlike OpenWRT, CapOS defaults to setting the network interface protocol to `DHCP` and `DHCPv6`.

The firewall by default accepts inbound requests from the `LAN` and rejects inbound requests from the `WAN` except for `2000/tcp` and `2020/tcp`.

## About

### Documentation

* [FWERKOR Blog (For CapOS)](https://blog.fwerkor.com/category/capos)

### Community

* [Github Issues](https://github.com/fwerkor/capos/issues): For bug feedback, feature update suggestions.
* [Github Discussions](https://github.com/fwerkor/capos/discussions): For bug feedback, feature update suggestions.

### License

CapOS is licensed under GPL-2.0. 
