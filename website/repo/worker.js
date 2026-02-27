function escapeHtml(str) {
  return str.replace(/[&<>"']/g, c => ({
    "&": "&amp;",
    "<": "&lt;",
    ">": "&gt;",
    '"': "&quot;",
    "'": "&#39;"
  }[c]));
}

function encodePath(path) {
  return path.split("/").map(encodeURIComponent).join("/");
}

function safeDecodeURIComponent(input) {
  try {
    return decodeURIComponent(input);
  } catch {
    return null;
  }
}

function getParentPath(prefix) {
  if (!prefix) return null;

  const parts = prefix.split("/").filter(Boolean);
  if (parts.length === 0) return null;

  parts.pop();
  return parts.length ? parts.join("/") + "/" : "";
}

function formatBytes(bytes) {
  if (typeof bytes !== "number" || Number.isNaN(bytes) || bytes < 0) return "-";
  if (bytes < 1024) return `${bytes} B`;

  const units = ["KB", "MB", "GB", "TB"];
  let value = bytes / 1024;
  let unitIndex = 0;

  while (value >= 1024 && unitIndex < units.length - 1) {
    value /= 1024;
    unitIndex += 1;
  }

  return `${value.toFixed(value >= 10 ? 0 : 1)} ${units[unitIndex]}`;
}

function formatDate(dateString) {
  if (!dateString) return "-";

  const date = new Date(dateString);
  if (Number.isNaN(date.getTime())) return "-";

  return date.toISOString().replace("T", " ").slice(0, 19) + " UTC";
}

// Fetch all paginated results
async function listAll(env, options) {
  let objects = [];
  let prefixes = [];
  let cursor;

  do {
    const res = await env.BUCKET.list({
      ...options,
      cursor
    });

    objects.push(...res.objects);
    prefixes.push(...(res.delimitedPrefixes || []));

    cursor = res.truncated ? res.cursor : undefined;
  } while (cursor);

  return { objects, delimitedPrefixes: prefixes };
}

function renderDirectoryIndex(prefix, list) {
  const safePrefix = escapeHtml(prefix);
  const parent = getParentPath(prefix);

  const directories = [...list.delimitedPrefixes].sort().map((dir) => {
    const name = dir.replace(prefix, "");
    return {
      name,
      href: `/${encodePath(dir)}`,
    };
  });

  const files = [...list.objects]
    .sort((a, b) => a.key.localeCompare(b.key))
    .map((obj) => {
      const name = obj.key.replace(prefix, "");
      return {
        name,
        href: `/${encodePath(obj.key)}`,
        size: formatBytes(obj.size),
        modified: formatDate(obj.uploaded),
      };
    })
    .filter((file) => file.name);

  const totalEntries = directories.length + files.length;

  let html = `<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Index of /${safePrefix}</title>
<style>
:root {
  color-scheme: light dark;
  --bg: #f5f7fb;
  --surface: #ffffff;
  --text: #1f2937;
  --muted: #6b7280;
  --line: #e5e7eb;
  --accent: #2563eb;
  --accent-soft: rgba(37, 99, 235, 0.1);
}
@media (prefers-color-scheme: dark) {
  :root {
    --bg: #0b1020;
    --surface: #11162a;
    --text: #e5e7eb;
    --muted: #9ca3af;
    --line: #232b45;
    --accent: #7aa2ff;
    --accent-soft: rgba(122, 162, 255, 0.16);
  }
}
* { box-sizing: border-box; }
body {
  margin: 0;
  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", "Courier New", monospace;
  background: radial-gradient(circle at 20% -10%, var(--accent-soft), transparent 42%), var(--bg);
  color: var(--text);
  min-height: 100vh;
}
main {
  max-width: 980px;
  margin: 0 auto;
  padding: 32px 20px 40px;
}
header {
  margin-bottom: 16px;
}
h1 {
  margin: 0;
  font-size: clamp(1.2rem, 2.5vw, 1.8rem);
}
.meta {
  margin-top: 8px;
  color: var(--muted);
  font-size: 0.92rem;
}
.card {
  background: var(--surface);
  border: 1px solid var(--line);
  border-radius: 12px;
  overflow: hidden;
  box-shadow: 0 10px 24px rgba(0, 0, 0, 0.08);
}
table {
  width: 100%;
  border-collapse: collapse;
}
th, td {
  text-align: left;
  padding: 12px 14px;
  border-bottom: 1px solid var(--line);
  vertical-align: middle;
}
th {
  color: var(--muted);
  font-size: 0.78rem;
  letter-spacing: 0.04em;
  text-transform: uppercase;
}
tr:last-child td {
  border-bottom: none;
}
a {
  color: inherit;
  text-decoration: none;
}
.name-link {
  display: inline-flex;
  align-items: center;
  gap: 10px;
  color: var(--accent);
}
.name-link:hover {
  text-decoration: underline;
}
.col-size,
.col-modified {
  color: var(--muted);
  white-space: nowrap;
}
.empty {
  text-align: center;
  padding: 36px;
  color: var(--muted);
}
footer {
  margin-top: 12px;
  font-size: 0.82rem;
  color: var(--muted);
}
.footer-link {
  color: inherit;
  text-decoration: underline;
  text-decoration-color: color-mix(in srgb, currentColor 65%, transparent);
}
.footer-link:hover {
  text-decoration-color: currentColor;
}
@media (max-width: 640px) {
  th.col-size,
  td.col-size,
  th.col-modified,
  td.col-modified {
    display: none;
  }
}
</style>
</head>
<body>
<main>
  <header>
    <h1>Index of /${safePrefix}</h1>
    <div class="meta">${totalEntries} item${totalEntries === 1 ? "" : "s"}</div>
  </header>
  <section class="card">
    <table>
      <thead>
        <tr>
          <th>Name</th>
          <th class="col-size">Size</th>
          <th class="col-modified">Last Modified</th>
        </tr>
      </thead>
      <tbody>`;

  if (parent !== null) {
    html += `<tr><td><a class="name-link" href="/${encodePath(parent)}">📁 ../</a></td><td class="col-size">-</td><td class="col-modified">-</td></tr>`;
  }

  for (const dir of directories) {
    html += `<tr><td><a class="name-link" href="${dir.href}">📁 ${escapeHtml(dir.name)}</a></td><td class="col-size">-</td><td class="col-modified">-</td></tr>`;
  }

  for (const file of files) {
    html += `<tr><td><a class="name-link" href="${file.href}">📄 ${escapeHtml(file.name)}</a></td><td class="col-size">${file.size}</td><td class="col-modified">${file.modified}</td></tr>`;
  }

  if (totalEntries === 0) {
    html += `<tr><td class="empty" colspan="3">This directory is empty.</td></tr>`;
  }

  html += `</tbody>
    </table>
  </section>
  <footer>Served by <a class="footer-link" href="https://capos.top">CapOS Project</a></footer>
</main>
</body>
</html>`;
  return html;
}

export default {
  async fetch(request, env) {
    const url = new URL(request.url);

    const rawPath = url.pathname.slice(1);
    const decoded = safeDecodeURIComponent(rawPath);
    if (decoded === null) {
      return new Response("Bad Request", { status: 400 });
    }

    const path = decoded;

    if (path.includes("..")) {
      return new Response("Forbidden", { status: 403 });
    }

    // Directory
    if (path === "" || path.endsWith("/")) {
      const prefix = path;

      const list = await listAll(env, {
        prefix,
        delimiter: "/"
      });

      const html = renderDirectoryIndex(prefix, list);

      return new Response(html, {
        headers: {
          "content-type": "text/html;charset=UTF-8",
          "Content-Security-Policy": "default-src 'self'; style-src 'unsafe-inline'; script-src 'none';"
        },
      });
    }

    // File
    const object = await env.BUCKET.get(path);
    if (!object) return new Response("Not Found", { status: 404 });

    return new Response(object.body, {
      headers: {
        "Content-Type": object.httpMetadata?.contentType || "application/octet-stream",
        "Content-Disposition": "inline"
      },
    });
  },
};

export { renderDirectoryIndex };
