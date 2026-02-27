export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    let path = decodeURIComponent(url.pathname.slice(1));

    // 目录浏览
    if (path === "" || path.endsWith("/")) {
      const prefix = path;
      const list = await env.BUCKET.list({ prefix });

      let html = `<h1>Index of /${prefix}</h1><ul>`;
      if (prefix !== "") {
        const parent = prefix.split("/").slice(0, -2).join("/") + "/";
        html += `<li><a href="/${parent}">../</a></li>`;
      }

      for (const obj of list.objects) {
        const name = obj.key.replace(prefix, "");
        if (name) {
          html += `<li><a href="/${obj.key}">${name}</a></li>`;
        }
      }
      html += "</ul>";

      return new Response(html, {
        headers: { "content-type": "text/html;charset=UTF-8" },
      });
    }

    // 文件下载
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