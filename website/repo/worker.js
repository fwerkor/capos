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

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    const rawPath = url.pathname.slice(1);
    const decoded = safeDecodeURIComponent(rawPath);

    if (decoded === null) {
      return new Response("Bad Request", { status: 400 });
    }

    let path = decoded;

    // 📂 DIRECTORY VIEW
    if (path === "" || path.endsWith("/")) {
      const prefix = path;

      const list = await env.BUCKET.list({
        prefix,
        delimiter: "/"
      });

      let html = `<h1>Index of /${escapeHtml(prefix)}</h1><ul>`;

      // 🔙 parent directory
      if (prefix !== "") {
        const parent = prefix.split("/").slice(0, -2).join("/") + "/";
        html += `<li><a href="/${encodePath(parent)}">../</a></li>`;
      }

      // 📁 subdirectories
      for (const dir of list.delimitedPrefixes || []) {
        const name = dir.replace(prefix, "");
        html += `<li>📁 <a href="/${encodePath(dir)}">${escapeHtml(name)}</a></li>`;
      }

      // 📄 files
      for (const obj of list.objects) {
        const name = obj.key.replace(prefix, "");
        if (!name) continue;

        html += `<li>📄 <a href="/${encodePath(obj.key)}">${escapeHtml(name)}</a></li>`;
      }

      html += "</ul>";

      return new Response(html, {
        headers: {
          "content-type": "text/html;charset=UTF-8",
          "Content-Security-Policy": "default-src 'self'; script-src 'none';"
        },
      });
    }

    // 📄 FILE DOWNLOAD
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