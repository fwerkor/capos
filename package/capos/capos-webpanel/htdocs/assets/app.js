"use strict";

const state = {
  session: null,
  apps: [],
  desktop: null,
  upload: null,
  pending: new Set(),
  portApp: null,
};

const $ = (id) => document.getElementById(id);

const nodes = {
  loginView: $("loginView"),
  loginForm: $("loginForm"),
  loginBtn: $("loginBtn"),
  username: $("username"),
  password: $("password"),
  appShell: $("appShell"),
  topbar: document.querySelector(".topbar"),
  controlDrawer: $("controlDrawer"),
  appsList: $("appsList"),
  appsCount: $("appsCount"),
  appSearch: $("appSearch"),
  appFilter: $("appFilter"),
  sessionSummary: $("sessionSummary"),
  userBadge: $("userBadge"),
  luciLink: $("luciLink"),
  inspectionBox: $("inspectionBox"),
  desktopHint: $("desktopHint"),
  desktopStatus: $("desktopStatus"),
  desktopFrame: $("desktopFrame"),
  toastStack: $("toastStack"),
  cpkFile: $("cpkFile"),
  uploadBtn: $("uploadBtn"),
  installBtn: $("installBtn"),
  portModal: $("portModal"),
  portForm: $("portForm"),
  portModalApp: $("portModalApp"),
  portListen: $("portListen"),
  portTarget: $("portTarget"),
  portProto: $("portProto"),
  savePortBtn: $("savePortBtn"),
  confirmModal: $("confirmModal"),
  confirmTitle: $("confirmTitle"),
  confirmMessage: $("confirmMessage"),
  confirmOkBtn: $("confirmOkBtn"),
};

class ApiError extends Error {
  constructor(message, status, code) {
    super(message);
    this.status = status;
    this.code = code;
  }
}

function h(value) {
  return String(value ?? "")
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#39;");
}

function showToast(message, kind = "info") {
  const toast = document.createElement("div");
  toast.className = `toast ${kind}`;
  toast.textContent = message;
  nodes.toastStack.appendChild(toast);
  setTimeout(() => toast.remove(), 3800);
}

function resetSessionUi(message) {
  state.session = null;
  state.apps = [];
  state.desktop = null;
  state.upload = null;
  nodes.password.value = "";
  nodes.controlDrawer.classList.remove("open");
  nodes.loginView.style.display = "grid";
  nodes.appShell.classList.remove("ready");
  nodes.desktopFrame.src = "about:blank";
  nodes.installBtn.disabled = true;
  nodes.inspectionBox.innerHTML = "<p>等待上传 CPK。</p>";
  if (message) {
    showToast(message, "error");
  }
}

async function api(path, options = {}) {
  const res = await fetch(`/cgi-bin/cap/api${path}`, {
    credentials: "same-origin",
    ...options,
  });
  const text = await res.text();
  let data = {};
  try {
    data = text ? JSON.parse(text) : {};
  } catch (error) {
    throw new ApiError(text || `HTTP ${res.status}`, res.status, "INVALID_JSON");
  }
  if (res.status === 401) {
    resetSessionUi(data.message || "登录状态已过期，请重新登录。");
    throw new ApiError(data.message || "unauthenticated", res.status, data.error_code || "UNAUTHENTICATED");
  }
  if (!res.ok || data.ok === false) {
    throw new ApiError(data.message || `HTTP ${res.status}`, res.status, data.error_code || "ERROR");
  }
  return data;
}

function setPending(key, pending) {
  if (pending) {
    state.pending.add(key);
  } else {
    state.pending.delete(key);
  }
  renderApps();
  nodes.loginBtn.disabled = state.pending.has("login");
  nodes.uploadBtn.disabled = state.pending.has("upload");
  nodes.installBtn.disabled = !state.upload || state.pending.has("install");
  nodes.savePortBtn.disabled = state.pending.has("port");
}

function isDesktopApp(appName) {
  return Boolean(state.desktop && state.desktop.app === appName);
}

function risksForApp(item) {
  const manifest = item.manifest || {};
  const container = manifest.container || {};
  const privileges = container.privileges || {};
  const network = manifest.network || {};
  const host = manifest.host || {};
  const exec = host.exec || {};
  const risks = [];
  if (network.host || item.permissions?.host_network) risks.push("host network");
  if (exec.enabled || item.permissions?.host_exec) risks.push("host.exec");
  if (privileges.enabled) risks.push("privileged");
  if ((privileges.capabilities || []).length) risks.push("capabilities");
  if ((container.devices || []).length) risks.push("devices");
  if ((container.volumes?.extra || []).length) risks.push("host mounts");
  if ((network.publish || []).length) risks.push("publish");
  return risks;
}

function serviceLabel(item) {
  const desktop = item.desktop || {};
  if (!desktop.port) {
    return "未声明";
  }
  return `${desktop.scheme || "http"}://${desktop.target_host || "-"}:${desktop.port}`;
}

function portmapText(map) {
  return `${map.proto || "tcp"} ${map.listen}->${map.target_port}`;
}

function appMatches(item, query, filter) {
  const app = item.app || {};
  const running = Boolean(item.runtime?.running);
  const desktop = isDesktopApp(app.name);
  const risks = risksForApp(item);
  if (filter === "running" && !running) return false;
  if (filter === "stopped" && running) return false;
  if (filter === "desktop" && !desktop) return false;
  if (filter === "risk" && risks.length === 0) return false;
  if (!query) return true;
  const haystack = [
    app.name,
    app.nickname,
    app.version,
    app.description,
    item.runtime?.container_name,
    item.runtime?.ip,
    serviceLabel(item),
    ...(item.portmaps || []).map(portmapText),
    ...risks,
  ]
    .filter(Boolean)
    .join(" ")
    .toLowerCase();
  return haystack.includes(query);
}

function emptyState(title, message) {
  return `<div class="empty-state"><h4>${h(title)}</h4><p>${h(message)}</p></div>`;
}

function renderApps() {
  const query = nodes.appSearch.value.trim().toLowerCase();
  const filter = nodes.appFilter.value;
  const apps = state.apps.filter((item) => appMatches(item, query, filter));
  nodes.appsCount.textContent = `${state.apps.length} 个应用`;

  if (!state.apps.length) {
    nodes.appsList.innerHTML = emptyState("还没有已安装应用", "上传 CPK 后，应用会显示在这里。");
    return;
  }
  if (!apps.length) {
    nodes.appsList.innerHTML = emptyState("没有匹配的应用", "调整搜索词或过滤条件。");
    return;
  }

  nodes.appsList.innerHTML = apps
    .map((item) => {
      const app = item.app || {};
      const runtime = item.runtime || {};
      const running = Boolean(runtime.running);
      const desktop = isDesktopApp(app.name);
      const risks = risksForApp(item);
      const actionPending = (name) => state.pending.has(`${name}:${app.name}`);
      const disabled = (name) => (actionPending(name) ? "disabled" : "");
      const portmaps = item.portmaps || [];
      return `
        <article class="app-card ${desktop ? "is-desktop" : ""}">
          <div class="app-title">
            <div>
              <h4>${h(app.nickname || app.name)}</h4>
              <p>${h(app.description || "这个应用还没有提供描述。")}</p>
            </div>
            <span class="chip ${running ? "running" : "stopped"}">${running ? "Running" : "Stopped"}</span>
          </div>
          <div class="chips">
            <span class="chip">Version ${h(app.version || "-")}</span>
            ${desktop ? '<span class="chip desktop-chip">当前桌面</span>' : ""}
            ${risks.map((risk) => `<span class="chip risk">${h(risk)}</span>`).join("")}
          </div>
          <div class="app-grid">
            <div class="fact"><strong>容器</strong><span>${h(runtime.container_name || "-")}</span></div>
            <div class="fact"><strong>IP</strong><span>${h(runtime.ip || "-")}</span></div>
            <div class="fact"><strong>Service</strong><span>${h(serviceLabel(item))}</span></div>
            <div class="fact"><strong>Image</strong><span>${h(runtime.image_ref || "-")}</span></div>
          </div>
          <div class="port-list">
            ${
              portmaps.length
                ? portmaps
                    .map(
                      (map) =>
                        `<button class="port-pill" type="button" data-action="remove-port" data-app="${h(app.name)}" data-listen="${h(map.listen)}" data-target="${h(map.target_port)}" data-proto="${h(map.proto || "tcp")}">${h(portmapText(map))} x</button>`
                    )
                    .join("")
                : '<span class="chip">无端口映射</span>'
            }
          </div>
          <div class="actions">
            <button class="secondary" data-action="start" data-app="${h(app.name)}" ${disabled("start")}>启动</button>
            <button class="secondary" data-action="stop" data-app="${h(app.name)}" ${disabled("stop")}>停止</button>
            <button class="secondary" data-action="restart" data-app="${h(app.name)}" ${disabled("restart")}>重启</button>
            <button class="secondary" data-action="reconcile" data-app="${h(app.name)}" ${disabled("reconcile")}>修复状态</button>
            <button class="secondary" data-action="portmap" data-app="${h(app.name)}">映射端口</button>
            <button class="secondary" data-action="desktop" data-app="${h(app.name)}" ${desktop ? "disabled" : ""}>设为桌面</button>
            <button class="danger" data-action="uninstall" data-app="${h(app.name)}" ${disabled("uninstall")}>卸载</button>
          </div>
        </article>
      `;
    })
    .join("");
}

function renderDesktopStatus() {
  const appName = state.desktop?.app;
  nodes.desktopHint.textContent = appName ? `当前桌面应用：${appName}` : "请选择一个已安装应用作为桌面应用。";

  if (!appName) {
    nodes.desktopStatus.className = "desktop-status visible";
    nodes.desktopStatus.textContent = "还没有设置桌面应用。打开右侧应用面板，选择一个已安装应用作为桌面。";
    return;
  }

  const item = state.apps.find((candidate) => candidate.app?.name === appName);
  if (!item) {
    nodes.desktopStatus.className = "desktop-status visible";
    nodes.desktopStatus.textContent = "当前桌面应用记录指向一个不存在的应用，请重新选择桌面应用。";
    return;
  }
  if (!item.runtime?.running) {
    nodes.desktopStatus.className = "desktop-status visible";
    nodes.desktopStatus.textContent = `${item.app?.nickname || appName} 尚未启动，启动后桌面区会自动代理它的 HTTP 服务。`;
    return;
  }
  if (!item.desktop?.port) {
    nodes.desktopStatus.className = "desktop-status visible";
    nodes.desktopStatus.textContent = `${item.app?.nickname || appName} 没有声明 network.service.http，无法作为桌面 iframe 代理。`;
    return;
  }
  nodes.desktopStatus.className = "desktop-status";
  nodes.desktopStatus.textContent = "";
}

function updateViewportOffsets() {
  const topbarHeight = nodes.topbar ? nodes.topbar.offsetHeight : 62;
  document.documentElement.style.setProperty("--workspace-top", `${topbarHeight + 24}px`);
}

function refreshDesktopFrame() {
  const appName = state.desktop?.app;
  const item = state.apps.find((candidate) => candidate.app?.name === appName);
  if (!appName || !item?.runtime?.running || !item.desktop?.port) {
    nodes.desktopFrame.src = "about:blank";
    return;
  }
  nodes.desktopFrame.src = `/cgi-bin/cap/app?ts=${Date.now()}`;
}

async function loadSession() {
  const data = await api("/session");
  if (!data.authenticated) {
    resetSessionUi();
    return false;
  }
  state.session = data.session;
  nodes.loginView.style.display = "none";
  nodes.appShell.classList.add("ready");
  nodes.userBadge.textContent = `${data.session.username}${data.session.is_sudo ? " · sudo" : ""}`;
  nodes.sessionSummary.textContent = `当前用户：${data.session.username} · ${data.session.is_sudo ? "具备 sudo 权限" : "普通用户权限"}`;
  nodes.luciLink.style.display = data.session.is_sudo ? "inline-flex" : "none";
  updateViewportOffsets();
  return true;
}

async function loadApps() {
  const data = await api("/apps");
  state.apps = data.apps || [];
  renderApps();
}

async function loadDesktop() {
  state.desktop = await api("/desktop");
}

async function refreshAll() {
  if (!(await loadSession())) {
    return;
  }
  await Promise.all([loadApps(), loadDesktop()]);
  renderApps();
  renderDesktopStatus();
  refreshDesktopFrame();
}

function formBody(values) {
  return {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8" },
    body: new URLSearchParams(values),
  };
}

function confirmDialog(title, message, danger = false) {
  nodes.confirmTitle.textContent = title;
  nodes.confirmMessage.textContent = message;
  nodes.confirmOkBtn.className = danger ? "danger" : "primary";
  return new Promise((resolve) => {
    const handler = () => {
      nodes.confirmModal.removeEventListener("close", handler);
      resolve(nodes.confirmModal.returnValue === "ok");
    };
    nodes.confirmModal.addEventListener("close", handler);
    nodes.confirmModal.showModal();
  });
}

async function appAction(app, action) {
  const labels = {
    start: "启动",
    stop: "停止",
    restart: "重启",
    reconcile: "修复状态",
    uninstall: "卸载",
  };
  const danger = action === "uninstall" || action === "stop";
  if (["stop", "restart", "uninstall"].includes(action)) {
    const ok = await confirmDialog(`${labels[action]} ${app}`, `确认${labels[action]}应用 ${app}？`, danger);
    if (!ok) return;
  }
  const key = `${action}:${app}`;
  if (state.pending.has(key)) return;
  setPending(key, true);
  try {
    const path = action === "reconcile" ? `/apps/${encodeURIComponent(app)}/reconcile` : `/apps/${encodeURIComponent(app)}/${action}`;
    await api(path, { method: "POST" });
    showToast(`${labels[action]}完成：${app}`, "success");
    await refreshAll();
  } finally {
    setPending(key, false);
  }
}

function openPortModal(app) {
  state.portApp = app;
  nodes.portModalApp.textContent = `应用：${app}`;
  nodes.portListen.value = "";
  nodes.portTarget.value = "";
  nodes.portProto.value = "tcp";
  nodes.portModal.showModal();
  nodes.portListen.focus();
}

function validPort(value) {
  return /^\d+$/.test(value) && Number(value) >= 1 && Number(value) <= 65535;
}

async function savePortMapping() {
  const app = state.portApp;
  if (!app) return;
  const listen = nodes.portListen.value.trim();
  const target = nodes.portTarget.value.trim();
  const proto = nodes.portProto.value;
  if (!validPort(listen) || !validPort(target)) {
    showToast("端口必须是 1-65535。", "error");
    return;
  }
  setPending("port", true);
  try {
    await api(`/apps/${encodeURIComponent(app)}/ports`, formBody({ listen, target, proto }));
    showToast(`已保存 ${app} 的 ${proto} ${listen}->${target}`, "success");
    nodes.portModal.close();
    await refreshAll();
  } finally {
    setPending("port", false);
  }
}

async function removePortMapping(button) {
  const app = button.dataset.app;
  const listen = button.dataset.listen;
  const target = button.dataset.target;
  const proto = button.dataset.proto || "tcp";
  const ok = await confirmDialog("删除端口映射", `删除 ${app} 的 ${proto} ${listen}->${target} 映射？`, true);
  if (!ok) return;
  await api(`/apps/${encodeURIComponent(app)}/ports`, formBody({ action: "remove", listen, target, proto }));
  showToast("端口映射已删除", "success");
  await refreshAll();
}

function reviewRiskItems(inspection) {
  const manifest = inspection.manifest || {};
  const container = manifest.container || {};
  const privileges = container.privileges || {};
  const network = manifest.network || {};
  const hostExec = manifest.host?.exec || {};
  const items = [];
  if (network.host || inspection.permissions?.host_network) items.push("使用 host network，容器与宿主机共享网络命名空间。");
  if (privileges.enabled || inspection.permissions?.privileged) items.push("请求 privileged 容器权限。");
  if ((container.devices || inspection.permissions?.devices || []).length) items.push(`请求设备访问：${(container.devices || inspection.permissions?.devices || []).join(", ")}`);
  if (hostExec.enabled || inspection.permissions?.host_exec) items.push("启用 host.exec，可按 token 调用宿主机白名单命令。");
  if ((container.volumes?.extra || []).length) items.push(`请求宿主机目录挂载：${container.volumes.extra.join(", ")}`);
  if ((network.publish || inspection.permissions?.publish || []).length) items.push("安装时会创建宿主机端口映射。");
  if ((privileges.capabilities || inspection.permissions?.capabilities || []).length) items.push(`额外 capabilities：${(privileges.capabilities || inspection.permissions?.capabilities || []).join(", ")}`);
  return items;
}

function renderInspection(inspection) {
  const app = inspection.app || {};
  const cpk = inspection.cpk || {};
  const manifest = inspection.manifest || {};
  const network = manifest.network || {};
  const env = manifest.container?.environment || [];
  const dependencies = manifest.dependencies || {};
  const risks = reviewRiskItems(inspection);
  nodes.inspectionBox.innerHTML = `
    <div>
      <h4>${h(app.nickname || app.name || "未命名应用")}</h4>
      <p>${h(app.description || "这个 CPK 没有提供描述。")}</p>
    </div>
    <div class="review-grid">
      <div class="fact"><strong>应用名</strong><span>${h(app.name || "-")}</span></div>
      <div class="fact"><strong>版本</strong><span>${h(app.version || "-")}</span></div>
      <div class="fact"><strong>架构</strong><span>${h(cpk.arch || "-")}</span></div>
      <div class="fact"><strong>镜像</strong><span>${h(cpk.image || "-")}</span></div>
      <div class="fact"><strong>Service</strong><span>${h(network.service?.http ? `http:${network.service.http}` : network.service?.https ? `https:${network.service.https}` : "未声明")}</span></div>
      <div class="fact"><strong>Publish</strong><span>${h(JSON.stringify(network.publish || []))}</span></div>
      <div class="fact"><strong>依赖</strong><span>${h(JSON.stringify(dependencies))}</span></div>
      <div class="fact"><strong>环境变量</strong><span>${h(JSON.stringify(env))}</span></div>
    </div>
    ${
      risks.length
        ? `<div class="risk-list">${risks.map((risk) => `<div class="risk-item">${h(risk)}</div>`).join("")}</div>`
        : '<p>未发现 privileged、devices、host.exec、host network 等高风险能力。</p>'
    }
  `;
}

async function uploadCpk() {
  const file = nodes.cpkFile.files[0];
  if (!file) {
    showToast("请先选择一个 CPK 文件。", "error");
    return;
  }
  setPending("upload", true);
  try {
    const data = await api(`/upload?filename=${encodeURIComponent(file.name)}`, {
      method: "POST",
      headers: { "Content-Type": "application/octet-stream" },
      body: await file.arrayBuffer(),
    });
    state.upload = data.upload;
    renderInspection(data.inspection || {});
    showToast("上传完成，预检通过。", "success");
  } catch (error) {
    state.upload = null;
    nodes.inspectionBox.innerHTML = `<div class="risk-item">${h(error.message)}</div>`;
    throw error;
  } finally {
    setPending("upload", false);
  }
}

async function installUpload() {
  if (!state.upload) {
    showToast("还没有可安装的上传结果。", "error");
    return;
  }
  const ok = await confirmDialog("确认安装 CPK", "安装会创建应用状态、载入镜像，并可能按 manifest 创建容器、端口映射和 hostexec token。确认继续？", true);
  if (!ok) return;
  setPending("install", true);
  try {
    await api("/install", formBody({ upload_id: state.upload.id }));
    nodes.inspectionBox.innerHTML = "<p>安装成功，可以在应用库中查看。</p>";
    nodes.cpkFile.value = "";
    state.upload = null;
    showToast("应用已安装。", "success");
    await refreshAll();
  } finally {
    setPending("install", false);
  }
}

nodes.loginForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  setPending("login", true);
  try {
    await api("/login", formBody({ username: nodes.username.value, password: nodes.password.value }));
    nodes.password.value = "";
    showToast("登录成功。", "success");
    await refreshAll();
  } catch (error) {
    showToast(error.message, "error");
  } finally {
    setPending("login", false);
  }
});

$("logoutBtn").addEventListener("click", async () => {
  try {
    await api("/logout", { method: "POST" });
  } catch (error) {
    showToast(error.message, "error");
  } finally {
    resetSessionUi();
  }
});

$("toggleDrawerBtn").addEventListener("click", () => nodes.controlDrawer.classList.toggle("open"));
$("closeDrawerBtn").addEventListener("click", () => nodes.controlDrawer.classList.remove("open"));
$("refreshBtn").addEventListener("click", () => refreshAll().catch((error) => showToast(error.message, "error")));
$("appsRefreshBtn").addEventListener("click", () => refreshAll().catch((error) => showToast(error.message, "error")));
nodes.appSearch.addEventListener("input", renderApps);
nodes.appFilter.addEventListener("change", renderApps);
window.addEventListener("resize", updateViewportOffsets);

nodes.appsList.addEventListener("click", async (event) => {
  const button = event.target.closest("button[data-action]");
  if (!button) return;
  const { action, app } = button.dataset;
  try {
    if (action === "remove-port") {
      await removePortMapping(button);
      return;
    }
    if (action === "portmap") {
      openPortModal(app);
      return;
    }
    if (action === "desktop") {
      await api("/desktop", formBody({ app }));
      showToast(`已将 ${app} 设为桌面应用。`, "success");
      await refreshAll();
      return;
    }
    await appAction(app, action);
  } catch (error) {
    showToast(error.message, "error");
  }
});

nodes.portForm.addEventListener("submit", async (event) => {
  if (event.submitter?.value === "cancel") {
    return;
  }
  event.preventDefault();
  try {
    await savePortMapping();
  } catch (error) {
    showToast(error.message, "error");
  }
});

nodes.uploadBtn.addEventListener("click", () => uploadCpk().catch((error) => showToast(error.message, "error")));
nodes.installBtn.addEventListener("click", () => installUpload().catch((error) => showToast(error.message, "error")));

refreshAll().catch((error) => {
  resetSessionUi();
  showToast(error.message, "error");
});
updateViewportOffsets();
