# CapOS

### A lightweight Linux server platform with a polished web desktop experience

<div align="center">

<p>
  <a href="https://github.com/fwerkor/capos/actions/workflows/os-sanity.yml"><img src="https://img.shields.io/github/actions/workflow/status/fwerkor/capos/os-sanity.yml?branch=main&style=for-the-badge" alt="OS Sanity"></a>
  <a href="https://github.com/fwerkor/capos/releases"><img src="https://img.shields.io/github/v/release/fwerkor/capos?include_prereleases&style=for-the-badge" alt="Releases"></a>
  <a href="https://github.com/fwerkor/capos/stargazers"><img src="https://img.shields.io/github/stars/fwerkor/capos?style=for-the-badge" alt="Stars"></a>
  <a href="https://github.com/fwerkor/capos?tab=License-1-ov-file"><img src="https://img.shields.io/badge/License-GPL2.0-blue.svg?style=for-the-badge" alt="License"></a>
  <a href="https://github.com/fwerkor/capos/commits/main/"><img src="https://img.shields.io/github/last-commit/fwerkor/capos?style=for-the-badge" alt="Last Commit"></a>
</p>

<img src="include/logo.png" alt="CapOS logo" width="220">

<p>
  Built for people who want a compact, approachable, and capable server system.<br>
  CapOS combines low resource usage, a browser-first management experience, and room to grow from simple setups to serious self-hosted workloads.
</p>

<p>
  <a href="https://capos.top"><strong>Website</strong></a> ·
  <a href="https://github.com/fwerkor/capos/releases"><strong>Download Releases</strong></a> ·
  <a href="https://repo.capos.top"><strong>Package Repository</strong></a> ·
  <a href="https://github.com/fwerkor/capos/wiki/Quick-start-guide"><strong>Quick Start</strong></a> ·
  <a href="https://github.com/fwerkor/capos/wiki"><strong>Documentation</strong></a>
</p>

</div>

> Maintained by the **FWERKOR Team**, especially **Castronaut**.

## Why CapOS

CapOS is an open-source server operating system focused on clarity, efficiency, and day-to-day usability. It is designed for homelabs, learning environments, edge devices, and compact servers where a full management workflow matters just as much as raw functionality.

Instead of feeling like a stripped-down build environment, CapOS aims to feel like a complete server platform from the moment it boots: easy to access, easy to manage, and flexible enough to expand as your needs change.

## Highlights

<table>
  <tr>
    <td width="50%">
      <h3>Lightweight by Design</h3>
      <p>Runs with a small footprint and works well across a broad range of hardware, from compact devices to full PCs.</p>
    </td>
    <td width="50%">
      <h3>Webdesktop Management</h3>
      <p>Manage the system from a browser through a more visual, approachable interface for settings, packages, and system status.</p>
    </td>
  </tr>
  <tr>
    <td width="50%">
      <h3>Scales with Your Setup</h3>
      <p>Start simple, then grow into richer services and more advanced workloads without leaving the same platform.</p>
    </td>
    <td width="50%">
      <h3>Beginner-Friendly</h3>
      <p>A clean command-line workflow and simplified management flow help new users get productive quickly.</p>
    </td>
  </tr>
  <tr>
    <td width="50%">
      <h3>Stable and Secure</h3>
      <p>Built on a mature Linux foundation with a strong focus on dependable behavior and practical security defaults.</p>
    </td>
    <td width="50%">
      <h3>Open Source</h3>
      <p>Released under GPL-2.0 and open to community contributions, improvements, and ecosystem growth.</p>
    </td>
  </tr>
</table>

## Quick Overview

| Area | Default |
| --- | --- |
| Web panel | `2000/tcp` (HTTP), `2020/tcp` (HTTPS) |
| Remote access | `23/tcp` for Telnet before a root password is set, `22/tcp` for SSH |
| Network mode | Uses `DHCP` and `DHCPv6` by default |
| Firewall | Accepts inbound traffic from `LAN`; rejects inbound traffic from `WAN` except `2000/tcp` and `2020/tcp` |

More usage details are available in the [User Guide](https://github.com/fwerkor/capos/wiki/User-guide).

## Downloads

| Resource | Link |
| --- | --- |
| Releases | [GitHub Releases](https://github.com/fwerkor/capos/releases) |
| Package repository | [repo.capos.top](https://repo.capos.top) |
| Quick start | [Wiki Quick Start Guide](https://github.com/fwerkor/capos/wiki/Quick-start-guide) |

## Development

CapOS welcomes contributors, maintainers, and application developers. If you want to improve the platform or build apps around it, the best starting points are below.

| Topic | Link |
| --- | --- |
| Developer guide | [CapOS Developer Guide](https://github.com/fwerkor/capos/wiki/Developer-guide) |
| App development | [CAPP Development Guide](https://blog.fwerkor.com/archives/1123) |
| Example project | [capp-helloworld](https://github.com/fwerkor/capp-helloworld) |

### Source Code Access

To make repository access easier across different regions, CapOS provides:

- Primary repository: https://github.com/fwerkor/capos
- Read-only mirror for users in Mainland China: https://gitcode.com/fwerkor/capos

Issues, discussions, and pull requests should be submitted through the GitHub primary repository.

### Build from Source

To build your own firmware, use a GNU/Linux, BSD, or macOS system with a case-sensitive filesystem. Cygwin is not supported.

Required tools vary by distribution, but the typical dependency set includes:

```text
binutils bzip2 diff find flex gawk gcc-6+ getopt grep install libc-dev libz-dev
make4.1+ perl python3.7+ rsync subversion unzip which
```

An automatic dependency installer is available at `scripts/auto_install_dependencies.sh`, but manual installation is recommended if the script does not match your environment.

Build steps:

1. Run `bash ./scripts/auto_install_dependencies.sh` to install dependencies.
2. Run `make menuconfig` to choose the target platform, toolchain, and packages.
3. Run `make` to build the firmware.

Use a non-root user for builds.

## About

### Documentation

- [GitHub Wiki](https://github.com/fwerkor/capos/wiki)
- [FWERKOR Blog](https://blog.fwerkor.com/category/capos)

### Community

- [GitHub Issues](https://github.com/fwerkor/capos/issues) for bug reports and feature requests
- [GitHub Discussions](https://github.com/fwerkor/capos/discussions) for questions, ideas, and community conversations

### License

CapOS is licensed under **GPL-2.0**.

## Project Activity

<p align="center">
  <img src="https://repobeats.axiom.co/api/embed/ed4a081fa92fc9600b18e5a14aaa4a6092da655c.svg" alt="Repobeats analytics image">
</p>

<p align="center">
  <a href="https://www.star-history.com/#fwerkor/capos&type=date&legend=top-left">
    <picture>
      <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/svg?repos=fwerkor/capos&type=date&theme=dark&legend=top-left" />
      <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/svg?repos=fwerkor/capos&type=date&legend=top-left" />
      <img alt="Star History Chart" src="https://api.star-history.com/svg?repos=fwerkor/capos&type=date&legend=top-left" />
    </picture>
  </a>
</p>
