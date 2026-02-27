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

// 🔥 NEW: Fetch all paginated results
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
  let html = `<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Index of /${escapeHtml(prefix)}</title>
<style>
body { font-family: monospace; padding: 20px; }
a { text-decoration: none; }
li { margin: 4px 0; }
</style>
</head>
<body>
<h1>Index of /${escapeHtml(prefix)}</h1>
<ul>`;

  const parent = getParentPath(prefix);
  if (parent !== null) {
    html += `<li>📁 <a href="/${encodePath(parent)}">../</a></li>`;
  }

  // sort directories
  list.delimitedPrefixes.sort();
  for (const dir of list.delimitedPrefixes) {
    const name = dir.replace(prefix, "");
    html += `<li>📁 <a href="/${encodePath(dir)}">${escapeHtml(name)}</a></li>`;
  }

  // sort files
  list.objects.sort((a, b) => a.key.localeCompare(b.key));
  for (const obj of list.objects) {
    const name = obj.key.replace(prefix, "");
    if (!name) continue;

    html += `<li>📄 <a href="/${encodePath(obj.key)}">${escapeHtml(name)}</a></li>`;
  }

  html += `</ul></body></html>`;
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

    // 📂 Directory
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
          "Content-Security-Policy": "default-src 'self'; script-src 'none';"
        },
      });
    }

    // 📄 File
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