# CapOS `capbox` / `capos-webpanel` 实现方案

## 1. 文档目的

本文件用于记录 `package/capos/capbox` 与 `package/capos/capos-webpanel` 的完整实现方案，作为后续开发的统一基线。

当前目标不是一次性做出“应用商店生态”，而是先把 CapOS 自带的应用运行与管理能力做扎实，包括：

- 基于 Podman 的多用户应用隔离运行环境
- 基于 CPK 的本地上传安装
- 基于 Linux 用户的鉴权和权限控制
- 基于 Web 面板的应用管理与桌面应用展示

## 2. 已确认的产品边界

### 2.1 已确认事实

- CapOS 基于 OpenWrt。
- Web UI 运行在 `2000/tcp` 和 `2020/tcp`。
- `/www/index.html` 已跳转到 `/cap/`。
- `uhttpd` 已启用 `/cgi-bin` 前缀，适合部署 CGI 程序。
- “用户”概念完全等价于 Linux 用户。
- 应用本质是 Podman 容器。
- 不同用户之间的应用库必须隔离。
- 同一用户不能重复安装同一个应用。
- 同一个应用名可以被不同用户分别安装。
- 每个用户的所有应用处于同一个虚拟网络中。
- 同一用户的应用之间可以通过“应用名”直接互访。
- 不同用户安装同名应用时，网络和运行时不能互相冲突。
- 应用默认不绑定宿主机端口，除非元数据声明或用户后续手动配置。
- 只有 sudo 用户能配置宿主机 `1024` 以下端口。
- 只有 sudo 用户能在线安装需要 `host network` 的应用。
- sudo 用户登录后能看到 LuCI 链接，普通用户不能。
- 每个用户都可以从自己的已安装应用中选一个“桌面应用”。

### 2.2 本轮新增确认

- “在线安装”不是软件源，不做内建应用商店。
- Web 面板中的“在线安装”实际含义是“用户上传 `.cpk` 文件并安装”。
- 更复杂的应用商店能力应由一个独立的“应用”来实现，而不是做进系统面板。
- 某些应用需要具备“在宿主机上以当前用户身份执行命令”的能力。

### 2.3 本轮不做的内容

- 不做完整应用商店。
- 不做远程仓库索引、搜索、推荐、评分、自动更新。
- 不做跨用户应用统一管理后台。
- 不做复杂的多节点或集群能力。

## 3. 仓库现状

### 3.1 现有文件

- `package/capos/capbox/Makefile`
- `package/capos/capbox/files/capbox`
- `package/capos/capos-webpanel/Makefile`
- `package/capos/capos-webpanel/src/api.cpp`
- `package/capos/capos-webpanel/src/app.cpp`
- `package/capos/capos-webpanel/htdocs/index.html`
- `files/www/index.html`

### 3.2 当前状态

- `capbox` 目前只有一个 `reload_portmap()` 的 bash 原型函数。
- `capos-webpanel` 目前仍是 Hello World 占位。
- `uhttpd` 默认监听 `2000/2020`，文档根目录是 `/www`，CGI 前缀为 `/cgi-bin`。
- `files/www/index.html` 已将根路径跳转到 `/cap/`。

### 3.3 现状判断

- 现有代码足以作为包骨架。
- `capbox` 和 `capos-webpanel` 的主体实现基本需要从头设计。
- 当前是适合统一定义架构、状态布局、权限模型和 API 边界的阶段。

## 4. 总体架构

### 4.1 设计原则

- 高权限操作只收敛在 `capbox`。
- `capos-webpanel` 不直接控制 Podman，而是通过 `capbox` 间接执行。
- 用户权限判断统一基于 Linux 用户与 sudo 能力。
- 容器运行时和 Web 鉴权状态分离。
- 所有涉及多用户隔离、安全边界、端口控制、宿主机命令执行的逻辑，必须在后端强制校验，不能只依赖前端限制。

### 4.2 模块分工

#### `capbox`

负责应用安装、校验、运行、停止、删除、端口映射、网络管理、桌面应用配置、宿主机受控命令执行等“系统级能力”。

#### `capos-webpanel`

负责用户登录、会话管理、权限判断、上传 CPK、展示应用列表、触发安装与管理动作、显示桌面应用、为 sudo 用户提供 LuCI 入口。

### 4.3 调用关系

用户浏览器 -> `uhttpd` -> `capos-webpanel` CGI -> `capbox` CLI -> Podman / 系统命令

### 4.4 为什么这样拆分

- 便于把高权限逻辑集中审计。
- 便于未来独立“应用商店”应用复用 `capbox` 能力。
- 便于在 CLI 和 Web 两侧共享同一套应用管理语义。

## 5. CPK 与元数据方案

## 5.1 CPK 格式

- CPK 本质是 `.tar.gz`。
- 包内核心文件是 `config.yaml`。
- 包内包含按架构区分的镜像 tar 文件，如 `image_amd64.tar`、`image_arm64.tar`。
- 可包含图标等附加文件，例如 `icon.jpg`。

### 5.2 已知元数据字段

根据当前参考格式，`config.yaml` 包含以下主要结构：

- `version`
- `app`
- `dependencies`
- `network`
- `container`
- `healthcheck`
- `build`

### 5.3 本轮必须支持的字段

#### `app`

- `name`
- `version`
- `nickname`
- `description`
- `source`
- `author`
- `vendor`
- `license`
- `docs`

#### `dependencies`

- `opkg`
- `capp`

#### `network`

- `host`
- `publish`
- `service.http`
- `service.https`
- `service.https_skip_check`
- `management.http`
- `management.https`
- `management.https_skip_check`

#### `container`

- `cmd.exe`
- `cmd.terminal`
- `volumes.data`
- `volumes.from`
- `volumes.extra`
- `tmpfs`
- `privileges.enabled`
- `privileges.capabilities`
- `privileges.allow_new_privs`
- `systemd`
- `resources.memory_reserved`
- `resources.shm_size`
- `environment`
- `devices`

#### `healthcheck`

- `enabled`
- `cmd`
- `interval`
- `retries`
- `start_period`
- `timeout`

#### `build`

- `architectures`
- `extra_files`

### 5.4 推荐的 1.1 增强字段与兼容策略

原则：

- `1.0` 旧格式继续兼容
- `1.1` 在不破坏旧包的前提下引入更结构化的写法
- `capbox` 内部统一做规范化，再进入安装和运行逻辑

建议从以下几项开始增强：

#### `network.publish`

兼容旧写法：

```yaml
network:
  publish:
    - "8080:80"
    - "5353:53/udp"
```

推荐新写法：

```yaml
network:
  publish:
    - listen: 8080
      target: 80
      proto: tcp
    - listen: 5353
      target: 53
      proto: udp
```

这样更容易做权限检查、冲突检查和未来扩展描述字段。

#### `container.environment`

兼容旧写法：

```yaml
container:
  environment:
    - PUID=1000
    - PGID=1000
```

推荐新写法：

```yaml
container:
  environment:
    PUID: "1000"
    PGID: "1000"
```

或者：

```yaml
container:
  environment:
    - name: PUID
      value: "1000"
    - name: PGID
      value: "1000"
```

#### `container.volumes.from`

旧设计里写成 `container-name:/path` 不够贴合 CapOS 的“应用”语义，而且 `podman --volumes-from` 实际也是面向整个容器。

推荐改成直接引用应用名：

```yaml
container:
  volumes:
    from:
      - postgres
```

或者：

```yaml
container:
  volumes:
    from:
      - app: postgres
```

#### `host.exec`

为了支持“应用可在宿主机上以当前用户身份执行命令”，建议在 manifest 中扩展一段自定义能力声明：

```yaml
host:
  exec:
    enabled: true
    allow:
      - /usr/bin/du
      - /usr/bin/rsync
```

也推荐支持对象化写法，便于以后补描述、参数策略或提示文案：

```yaml
host:
  exec:
    enabled: true
    allow:
      - command: /usr/bin/du
      - command: /usr/bin/rsync
```

#### 桌面入口规则

- 桌面应用始终使用 `network.service` 作为显示入口
- `network.management` 保留给应用自己的管理界面语义，不参与 WebPanel 的桌面显示选择

说明：

- 这不是容器直接拿宿主机 shell。
- 这是通过 `capbox` 提供的受控执行能力。
- 需要白名单、参数校验、超时与权限约束。

### 5.5 manifest 校验规则

- `app.name` 必须匹配 `^[a-z0-9_]+$`
- `app.name` 作为同一用户范围内唯一应用标识
- `version` 必须存在，当前接受 `1.0` 和 `1.1`
- 目标架构必须存在对应镜像 tar
- 同一用户已安装同名应用时拒绝安装
- `network.host=true` 时必须判定当前用户是否有权限
- `network.publish` 同时兼容字符串写法与对象写法，并逐项校验
- `dependencies.capp` 必须校验是否已在同一用户下安装
- `container.environment` 同时兼容字符串数组、键值对象和 `{name,value}` 对象数组
- `container.volumes.from` 只能引用当前用户下已存在应用，并推荐直接使用应用名
- `host.exec.enabled=true` 时安装页必须标记为高风险能力

## 6. 多用户隔离与命名策略

### 6.1 用户等价于 Linux 用户

- 登录用户名就是系统用户名。
- 所有应用归属某个 Linux 用户。
- Web 面板权限直接从当前登录用户的系统身份推导。

### 6.2 容器真实命名

同名应用允许被不同用户安装，因此容器真实名字不能直接等于应用名。

建议命名：

- 容器：`capbox_u_<uid>_<app>`
- 网络：`capbox_u_<uid>`
- 数据目录：`/var/lib/capbox/users/<uid>/apps/<app>/...`

### 6.3 网络别名与主机名

- 容器加入所属用户的独立网络。
- 在该网络中将应用名 `app.name` 作为 `hostname` / network alias。
- 这样同一用户内可以直接使用应用名互访。
- 不同用户由于处于不同网络，互不冲突。

### 6.4 隔离效果

示例：

- A 用户安装 `jellyfin`
- B 用户安装 `jellyfin`
- A 用户的 `nginx_proxy_manager` 可以通过 `http://jellyfin` 访问 A 用户的 `jellyfin`
- B 用户的应用不会访问到 A 用户的 `jellyfin`

## 7. 本地状态与目录布局

### 7.1 持久化状态目录

建议使用：

- `/var/lib/capbox/users/<uid>/profile.json`
- `/var/lib/capbox/users/<uid>/desktop_app`
- `/var/lib/capbox/users/<uid>/apps/<app>/manifest.yaml`
- `/var/lib/capbox/users/<uid>/apps/<app>/manifest.json`
- `/var/lib/capbox/users/<uid>/apps/<app>/meta.json`
- `/var/lib/capbox/users/<uid>/apps/<app>/icon.*`
- `/var/lib/capbox/users/<uid>/apps/<app>/portmaps.json`
- `/var/lib/capbox/users/<uid>/apps/<app>/hostexec.json`
- `/var/lib/capbox/users/<uid>/apps/<app>/data/`

### 7.2 运行时目录

建议使用：

- `/run/capbox/portmap/`
- `/run/capbox/sessions/` 或交由 webpanel 管理
- `/run/capbox/hostexec/`

### 7.3 状态原则

- `capbox` 必须以磁盘状态为准，而不是进程内存。
- 当前 `reload_portmap()` 中的 bash 关联数组只能作为原型，不适合正式持久化。
- 任何重启后的恢复逻辑都应来自磁盘状态 + Podman 实际状态。

## 8. `capbox` 详细方案

### 8.1 定位

`capbox` 是 CapOS 应用运行时的唯一系统级后端入口。

### 8.2 建议子命令

- `capbox app inspect <file>`
- `capbox app validate <file> [--user <name>]`
- `capbox app install <file> [--user <name>]`
- `capbox app list [--user <name>]`
- `capbox app info <app> [--user <name>]`
- `capbox app start <app> [--user <name>]`
- `capbox app stop <app> [--user <name>]`
- `capbox app restart <app> [--user <name>]`
- `capbox app uninstall <app> [--user <name>]`
- `capbox portmap list <app> [--user <name>]`
- `capbox portmap set <app> ...`
- `capbox portmap remove <app> ...`
- `capbox desktop get [--user <name>]`
- `capbox desktop set <app> [--user <name>]`
- `capbox hostexec run <app> ...`
- `capbox reconcile [--user <name>]`

### 8.3 语言选择

第一阶段建议继续使用 shell 脚本实现 `capbox`，依赖：

- `bash`
- `jq`
- `yq`
- `podman`
- `nsenter`
- `socat`

原因：

- 与现有代码连续性最好。
- OpenWrt 打包简单。
- 先把能力做通，再决定是否迁移到更强类型的实现语言。

### 8.4 `capbox` 的核心职责

#### CPK 预检

- 解压到临时目录
- 读取并校验 `config.yaml`
- 检查架构匹配
- 输出标准化的 manifest JSON
- 输出风险摘要

#### 安装

- 检查当前用户是否已装同名应用
- 检查 opkg 依赖
- 检查 CAPP 依赖
- 导入容器镜像
- 初始化用户网络
- 创建应用数据目录
- 按 manifest 创建容器
- 保存本地状态

#### 生命周期管理

- 列表
- 查询详情
- 启动
- 停止
- 重启
- 卸载

#### 网络与端口

- 维护每用户独立网络
- 管理宿主机端口映射
- 处理容器间卷引用

#### 桌面应用状态

- 读写用户当前桌面应用配置

#### 宿主机受控命令执行

- 按应用声明开放能力
- 以应用所属 Linux 用户身份执行
- 限制命令范围与参数范围

### 8.5 容器创建策略

默认策略：

- 默认桥接到用户专属网络
- 默认不暴露宿主机端口
- `network.host=true` 时改为 host 网络
- 若声明数据目录，则映射到应用专属数据路径
- 若声明终端命令，则保存以供后续使用

### 8.6 风险项分级

以下能力建议在安装前明确标记：

- `network.host=true`
- 申请宿主机低位端口
- `container.privileges.enabled=true`
- 请求 `devices`
- 请求宿主机额外挂载
- 启用 `host.exec`

其中：

- `network.host=true` 的本地上传安装仅允许 sudo 用户继续
- 低位端口映射仅允许 sudo 用户设置
- 对于高风险 manifest，安装界面必须显示风险说明

### 8.7 `reconcile` 机制

引入 `capbox reconcile`，用于：

- 重建端口映射
- 修复应用状态文件与 Podman 实际状态不一致的问题
- 系统重启后恢复运行所需辅助进程

## 9. 端口映射方案

### 9.1 现有基础

当前 `capbox` 已有一个 `reload_portmap()` 原型，底层思路是：

- `podman inspect` 找到容器 PID
- 使用 `nsenter -n`
- 使用 `socat` 将宿主机端口转发到容器网络命名空间内的 `127.0.0.1:<target>`

### 9.2 正式方案

- 将端口映射声明持久化到应用状态目录中
- 由 `capbox reconcile` 负责根据状态重建映射
- 不再依赖单一 bash 进程内的 PID 关联数组作为真实状态源

### 9.3 规则

- 默认无端口映射
- 用户可为自己的应用配置额外端口映射
- 普通用户只能使用宿主机 `>=1024` 端口
- sudo 用户可使用 `1-65535` 全部端口
- 端口冲突必须检测并报错
- TCP/UDP 分开管理

### 9.4 元数据中的 `network.publish`

- 安装时读取
- 逐项做权限判断和冲突判断
- 通过后转成正式状态
- 与后续用户手动添加的映射统一进入同一数据结构

## 10. 宿主机命令执行能力方案

### 10.1 目标

允许某些应用代表“当前应用所属用户”在宿主机上执行有限的命令。

### 10.2 严格限制

- 不直接把宿主机 shell 或 `podman.sock` 暴露给容器
- 不默认授予 root
- 仅以应用所属 Linux 用户身份执行
- 必须有 manifest 显式声明
- 建议必须有命令白名单
- 必须支持超时
- 必须记录日志

### 10.3 推荐实现

第一阶段先落地 HTTP token bridge，后续再考虑收敛到 Unix socket。

#### 第一阶段：HTTP token bridge

- 安装启用 `host.exec` 的应用时，为该应用生成独立 token
- 容器启动时注入：
  - `CAPOS_HOSTEXEC_URL`
  - `CAPOS_HOSTEXEC_TOKEN_FILE`
  - `CAPOS_HOSTEXEC_APP`
  - `CAPOS_HOSTEXEC_USER`
- 应用向 `/cgi-bin/cap/api/hostexec` 发起 `POST`
- 通过 `X-CapOS-App`、`X-CapOS-User`、`X-CapOS-Token` 鉴权
- 请求体第一行是绝对路径命令，后续每行一个参数
- `capos-webpanel` 调用 `capbox hostexec invoke`
- `capbox` 校验 token、应用归属、命令白名单，再以所属 Linux 用户身份执行
- 返回 JSON：`ok / exit_code / success / output`

#### 第二阶段：per-user 或 per-app socket

- 为每个用户暴露一个受控 Unix socket
- 应用通过 socket 请求执行宿主机命令
- socket 后面仍然由 `capbox` 统一处理

HTTP bridge 先把能力跑通，socket 方案后续再优化安全性和体验。

### 10.4 建议限制项

- 只允许执行白名单中的绝对路径命令
- 参数个数和参数格式可限制
- 指定工作目录白名单
- 指定环境变量白名单
- 默认禁止 shell 拼接
- 记录发起应用、用户、命令、退出码、耗时

## 11. `capos-webpanel` 详细方案

### 11.1 定位

`capos-webpanel` 是 CapOS 自带的轻量 WebDesktop 面板，负责用户认证和应用管理，不承担“应用商店”角色。

### 11.2 组成

- `src/api.cpp`：JSON API CGI
- `src/app.cpp`：桌面应用代理 CGI
- `htdocs/`：静态前端

### 11.3 功能范围

- 登录 / 登出
- 当前用户信息
- sudo 权限判断
- 应用列表与详情
- 上传 CPK
- CPK 预检
- 安装应用
- 启停重启卸载
- 端口映射管理
- 桌面应用选择
- 桌面应用显示
- sudo 用户显示 LuCI 链接

### 11.4 不承担的职责

- 不做 Podman 直接调用
- 不做远程仓库管理
- 不做复杂商店逻辑
- 不做跨用户统一总控台

## 12. 鉴权与会话方案

### 12.1 用户来源

- 直接使用系统 Linux 用户
- 不单独维护 Web 用户数据库

### 12.2 登录方式

建议第一阶段采用表单登录：

- 用户名
- 密码

服务端校验系统身份，成功后发 session cookie。

### 12.3 session 内容

建议包含：

- `session_id`
- `uid`
- `username`
- `is_sudo`
- `issued_at`
- `expires_at`

### 12.4 session 存储

建议存放在：

- `/run/capos-webpanel/sessions/<session_id>.json`

### 12.5 sudo 判断

需要服务端可靠判断用户是否具备 sudo 权限。

建议：

- 不只看前端传参
- 不只依赖前端界面隐藏
- 在服务端启动安装、端口绑定、host network 等敏感动作前重新检查

### 12.6 鉴权原则

- 前端所有权限显示仅作为体验优化
- 最终授权必须由 `api.cpp` 和 `capbox` 双重检查

## 13. Web API 设计

### 13.1 路径建议

- `/cgi-bin/cap/api`
- `/cgi-bin/cap/app`

### 13.2 API 风格

建议采用 JSON over CGI，按动作分发。

例如：

- `POST /cgi-bin/cap/api/login`
- `POST /cgi-bin/cap/api/logout`
- `GET /cgi-bin/cap/api/me`
- `GET /cgi-bin/cap/api/apps`
- `GET /cgi-bin/cap/api/apps/<app>`
- `POST /cgi-bin/cap/api/apps/upload`
- `POST /cgi-bin/cap/api/apps/install`
- `POST /cgi-bin/cap/api/apps/<app>/start`
- `POST /cgi-bin/cap/api/apps/<app>/stop`
- `POST /cgi-bin/cap/api/apps/<app>/restart`
- `DELETE /cgi-bin/cap/api/apps/<app>`
- `GET /cgi-bin/cap/api/apps/<app>/ports`
- `POST /cgi-bin/cap/api/apps/<app>/ports`
- `DELETE /cgi-bin/cap/api/apps/<app>/ports`
- `GET /cgi-bin/cap/api/desktop`
- `POST /cgi-bin/cap/api/desktop`

### 13.3 上传安装流程

建议拆成两步：

#### 第一步：上传并预检

- 用户上传 `.cpk`
- 服务端写入临时目录
- 调用 `capbox cpk inspect/validate`
- 返回 manifest 摘要和风险提示

#### 第二步：用户确认安装

- 前端展示关键信息
- 用户确认后执行安装
- 服务端调用 `capbox cpk install`

### 13.4 返回内容建议

统一返回：

- `ok`
- `message`
- `data`
- `error_code`

### 13.5 错误码建议

- `UNAUTHENTICATED`
- `FORBIDDEN`
- `INVALID_MANIFEST`
- `APP_ALREADY_INSTALLED`
- `PORT_CONFLICT`
- `HOST_NETWORK_NOT_ALLOWED`
- `LOW_PORT_NOT_ALLOWED`
- `DEPENDENCY_MISSING`
- `ARCH_NOT_SUPPORTED`

## 14. 前端 UI 方案

### 14.1 总体布局

- 顶部导航栏
- 左侧或顶部应用入口区
- 主显示区作为桌面应用承载区

### 14.2 顶部导航内容

- CapOS 标识
- 当前用户
- 应用列表
- 上传安装
- 端口映射管理
- 桌面应用选择
- LuCI 链接，仅 sudo 可见
- 退出登录

### 14.3 主显示区

- 若已设置桌面应用，则显示该应用内容
- 若未设置，则显示引导页
- 若应用未运行或无可展示入口，则显示状态说明

### 14.4 桌面应用选择逻辑

每个用户可以从自己的已安装应用中选一个桌面应用。

显示入口优先级：

- `service.http`
- `service.https`

### 14.5 安装界面展示内容

- 应用名
- 显示名
- 版本
- 作者
- 描述
- 风险项
- 是否请求 host network
- 是否请求宿主机命令执行
- 是否请求特权能力

## 15. 桌面应用代理方案

### 15.1 `app.cpp` 的作用

`app.cpp` 负责把用户当前选中的桌面应用接到 Web 面板内显示。

### 15.2 工作方式

- 读取当前 session 对应用户
- 读取该用户的桌面应用配置
- 解析应用目标端口
- 找到目标容器
- 代理请求到该容器服务

### 15.3 代理目标的选择

优先级：

- `service.http`
- `service.https`

### 15.4 需要注意的问题

- WebSocket 支持
- 绝对路径资源
- 反向代理头
- HTTPS 证书跳过策略
- 应用自身是否假设运行在根路径

第一阶段先保证基础 HTTP/HTTPS 管理页可代理。

## 16. 安装流程细化

### 16.1 上传

- 用户通过 Web 页面上传 `.cpk`
- 写入临时目录

### 16.2 预检

- 解包
- 校验 manifest
- 校验架构
- 校验依赖
- 校验权限要求
- 输出风险摘要

### 16.3 确认

- 用户确认安装

### 16.4 导入与创建

- `podman load` 导入镜像
- 创建用户网络
- 创建应用数据目录
- 创建容器
- 保存状态

### 16.5 启动

- 若策略允许，直接启动
- 写入桌面候选列表

## 17. 卸载流程细化

- 停止容器
- 删除容器
- 删除端口映射
- 删除应用状态目录
- 若当前桌面应用就是它，则清空桌面配置
- 保留或删除数据目录需要明确策略

建议第一阶段：

- 卸载时默认同时删除应用运行状态
- 对数据目录行为做明确提示

## 18. 安全策略

### 18.1 核心原则

- 不信任前端
- 不信任上传 manifest 的所有声明
- 不把 root 级控制面直接暴露给容器

### 18.2 必须后端校验的内容

- 当前用户身份
- sudo 权限
- 应用归属
- 应用名合法性
- 同名安装冲突
- 低位端口权限
- host network 权限
- `devices` 和危险挂载
- `host.exec` 是否允许

### 18.3 容器高危能力

以下项目应被视为高风险：

- `privileged`
- `host network`
- 挂宿主机系统目录
- 设备直通
- 宿主机命令执行

### 18.4 审计建议

建议记录：

- 登录
- 上传 CPK
- 安装确认
- 卸载
- 端口变更
- 桌面应用切换
- hostexec 请求

## 19. OpenWrt 打包与依赖

### 19.1 `capbox` 依赖建议

当前已有：

- `bash`
- `coreutils`
- `jq`
- `podman`
- `nsenter`
- `socat`

建议新增：

- `yq`

### 19.2 `capos-webpanel` 依赖建议

当前已有：

- `libstdcpp`
- `luci`
- `uhttpd`

视最终实现需要再补：

- 可能需要支持 multipart 上传解析的辅助实现
- 如需更稳健 JSON 处理，可考虑引入轻量依赖或自写最小解析

## 20. 开发分阶段计划

## Phase 1: `capbox` 核心框架

- [ ] 重构 `capbox` 为多子命令 CLI
- [ ] 引入 manifest 解析与校验
- [ ] 建立用户状态目录
- [ ] 实现用户网络创建与查询
- [ ] 实现应用列表、安装、卸载、启停
- [ ] 将现有端口映射原型纳入正式状态管理

## Phase 2: 上传安装闭环

- [ ] `api.cpp` 实现登录与 session
- [ ] 实现当前用户与 sudo 状态查询
- [ ] 实现 CPK 上传
- [ ] 实现预检接口
- [ ] 实现确认安装接口
- [ ] 实现错误码和统一 JSON 返回

## Phase 3: 应用管理 UI

- [ ] 前端应用列表
- [ ] 前端上传安装页面
- [ ] 风险项展示
- [ ] 应用启停卸载
- [ ] 端口映射管理
- [ ] sudo 用户显示 LuCI

## Phase 4: 桌面应用体验

- [ ] 用户可选择桌面应用
- [ ] `app.cpp` 代理桌面应用
- [ ] 未设置桌面应用时显示引导页
- [ ] 应用不可达时显示状态信息

## Phase 5: 宿主机受控命令执行

- [ ] 定义 manifest 扩展字段
- [ ] 在安装预检中展示该能力
- [ ] 实现 `capbox hostexec`
- [ ] 增加白名单和超时机制
- [ ] 增加日志记录

## Phase 6: 收尾与验证

- [ ] 完整错误处理
- [ ] 权限边界回归测试
- [ ] 多用户网络隔离测试
- [ ] 同名应用跨用户安装测试
- [ ] 低位端口权限测试
- [ ] host network 权限测试

## 21. 验收标准

### 21.1 应用与用户隔离

- A/B 用户可分别安装同名应用
- 同一用户不能重复安装同名应用
- 同一用户应用间可通过应用名互访
- 不同用户的同名应用不会串网

### 21.2 权限

- 普通用户看不到 LuCI
- sudo 用户能看到 LuCI
- 普通用户不能绑定低位端口
- sudo 用户可以绑定低位端口
- 普通用户不能安装 `host network` 应用
- sudo 用户可以安装 `host network` 应用

### 21.3 安装闭环

- 可上传 `.cpk`
- 可显示预检结果
- 可确认安装
- 可启动、停止、卸载

### 21.4 桌面应用

- 用户可以从自己的应用中选择桌面应用
- Web 面板可显示该应用
- 切换桌面应用后即时生效

### 21.5 宿主机命令执行

- 支持声明式开启
- 默认关闭
- 仅以应用所属用户身份执行
- 支持白名单和审计

## 22. 后续演进方向

- 独立“应用商店”应用复用 `capbox`
- 更细粒度的 manifest policy
- 更强的代理兼容性
- 更完善的应用图标、描述和分类展示
- 应用健康状态可视化
- 应用日志查看
- 应用升级与迁移流程

## 23. 当前结论

本轮实施基线如下：

- 先把 `capbox` 做成真正可用的系统级应用运行时后端。
- `capos-webpanel` 做成轻量但完整的用户面板。
- 面板内“在线安装”只表示“上传 CPK 并安装”。
- 复杂商店能力不做进系统面板。
- “应用可在宿主机上以当前用户身份执行命令”作为显式、受控、高风险能力接入。

后续开发均以本文件为准，若产品边界变化，再同步更新本文件。
