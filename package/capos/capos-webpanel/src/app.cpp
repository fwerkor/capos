#include "common.hpp"

#include <netdb.h>
#include <sys/socket.h>

using namespace capos;

extern char** environ;

namespace {

struct UpstreamResponse {
    int status = 502;
    std::string reason = "Bad Gateway";
    std::vector<std::pair<std::string, std::string>> headers;
    std::string body;
};

std::string effectivePathInfo() {
    auto path = getenvOrEmpty("PATH_INFO");
    if (!path.empty()) {
        return path;
    }

    auto uri = getenvOrEmpty("REQUEST_URI");
    const auto question = uri.find('?');
    if (question != std::string::npos) {
        uri = uri.substr(0, question);
    }
    const std::string prefix = "/cgi-bin/cap/app";
    if (uri.rfind(prefix, 0) == 0) {
        return uri.substr(prefix.size());
    }
    return "";
}

std::string proxyPrefix() {
    return "/cgi-bin/cap/app";
}

std::string requestScheme() {
    auto scheme = getenvOrEmpty("REQUEST_SCHEME");
    std::transform(scheme.begin(), scheme.end(), scheme.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (scheme == "http" || scheme == "https") {
        return scheme;
    }

    auto https = getenvOrEmpty("HTTPS");
    std::transform(https.begin(), https.end(), https.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (https == "on" || https == "1" || https == "yes" || https == "true") {
        return "https";
    }
    if (https == "off" || https == "0" || https == "no" || https == "false") {
        return "http";
    }

    if (getenvOrEmpty("SERVER_PORT") == "443") {
        return "https";
    }

    return "http";
}

std::string filterForwardCookies(const std::string& cookieHeader, const std::string& panelSessionId) {
    std::stringstream stream(cookieHeader);
    std::string item;
    std::vector<std::string> kept;
    bool removedPanelSessionCookie = false;

    while (std::getline(stream, item, ';')) {
        const auto trimmed = trim(item);
        if (trimmed.empty()) {
            continue;
        }
        const auto pos = trimmed.find('=');
        if (pos == std::string::npos) {
            kept.push_back(trimmed);
            continue;
        }
        const auto key = trim(trimmed.substr(0, pos));
        const auto value = trim(trimmed.substr(pos + 1));
        if (!removedPanelSessionCookie && key == kSessionCookieName && value == panelSessionId) {
            removedPanelSessionCookie = true;
            continue;
        }
        kept.push_back(trimmed);
    }

    std::ostringstream filtered;
    for (size_t i = 0; i < kept.size(); ++i) {
        if (i != 0) {
            filtered << "; ";
        }
        filtered << kept[i];
    }
    return filtered.str();
}

std::string urlEncode(const std::string& input) {
    std::ostringstream out;
    for (unsigned char ch : input) {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') || ch == '-' || ch == '_' || ch == '.' || ch == '~') {
            out << static_cast<char>(ch);
        } else if (ch == ' ') {
            out << '+';
        } else {
            out << '%' << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(ch) << std::nouppercase << std::dec << std::setfill(' ');
        }
    }
    return out.str();
}

std::string joinQuery(const std::map<std::string, std::string>& params, const std::vector<std::string>& exclude = {}) {
    std::ostringstream out;
    bool first = true;
    for (const auto& [key, value] : params) {
        if (std::find(exclude.begin(), exclude.end(), key) != exclude.end()) {
            continue;
        }
        if (!first) {
            out << '&';
        }
        first = false;
        out << urlEncode(key) << '=' << urlEncode(value);
    }
    return out.str();
}

std::string htmlShell(const std::string& title, const std::string& body) {
    std::ostringstream out;
    out << "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
        << "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        << "<title>" << htmlEscape(title) << "</title>"
        << "<style>"
        << ":root{color-scheme:light;"
        << "--bg:#f2f0ea;--panel:#fffaf0;--ink:#192126;--muted:#61717d;--line:#d8d2c5;--accent:#ca4e2f;--accent-soft:#ffe0d2;}"
        << "html,body{height:100%;margin:0;font-family:'Segoe UI',sans-serif;background:radial-gradient(circle at top,#fff9ef, #ece7dd 72%);color:var(--ink);}body{display:flex;min-height:100%;}"
        << ".shell{display:flex;flex-direction:column;gap:18px;width:100%;padding:22px;box-sizing:border-box;}.card{background:rgba(255,250,240,.92);border:1px solid var(--line);border-radius:20px;box-shadow:0 16px 40px rgba(45,53,59,.08);}"
        << ".empty{padding:28px;display:flex;flex-direction:column;gap:10px;}.empty h1{margin:0;font-size:22px;}.empty p{margin:0;color:var(--muted);line-height:1.5;}"
        << ".meta{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:12px;padding:0 28px 24px;}.meta div{background:#fff;border:1px solid var(--line);border-radius:14px;padding:12px 14px;}"
        << ".meta strong{display:block;font-size:12px;text-transform:uppercase;letter-spacing:.08em;color:var(--muted);margin-bottom:6px;}"
        << ".actions{display:flex;gap:12px;flex-wrap:wrap;padding:0 28px 24px;}.button{display:inline-flex;align-items:center;justify-content:center;border-radius:999px;padding:10px 16px;background:var(--accent);color:#fff;text-decoration:none;font-weight:700;}"
        << ".hint{padding:0 28px 24px;color:var(--muted);}.badge{display:inline-flex;align-items:center;padding:6px 10px;border-radius:999px;background:var(--accent-soft);color:var(--accent);font-weight:700;font-size:12px;}"
        << ".json{margin:0 28px 28px;background:#171c21;color:#dce4eb;border-radius:16px;padding:16px;overflow:auto;font:12px/1.5 ui-monospace,monospace;}"
        << "</style></head><body><main class=\"shell\">" << body << "</main></body></html>";
    return out.str();
}

std::string renderEmpty(const std::string& heading, const std::string& message, const std::string& extra = {}) {
    return htmlShell(
        heading,
        "<section class=\"card empty\">"
        "<h1>" + htmlEscape(heading) + "</h1>"
        "<p>" + htmlEscape(message) + "</p>"
        + extra +
        "</section>"
    );
}

std::optional<int> findMappedListenPort(const std::string& json, long long targetPort) {
    const std::regex pattern("\\{\\s*\"proto\"\\s*:\\s*\"([^\"]+)\"\\s*,\\s*\"listen\"\\s*:\\s*([0-9]+)\\s*,\\s*\"target_port\"\\s*:\\s*([0-9]+)\\s*\\}");
    for (std::sregex_iterator it(json.begin(), json.end(), pattern), end; it != end; ++it) {
        const auto proto = (*it)[1].str();
        const auto listen = std::stoi((*it)[2].str());
        const auto target = std::stoll((*it)[3].str());
        if (proto == "tcp" && target == targetPort) {
            return listen;
        }
    }
    return std::nullopt;
}

std::string replaceAll(std::string text, const std::string& from, const std::string& to) {
    if (from.empty()) {
        return text;
    }
    size_t pos = 0;
    while ((pos = text.find(from, pos)) != std::string::npos) {
        text.replace(pos, from.size(), to);
        pos += to.size();
    }
    return text;
}

std::string rewriteHtmlBody(std::string body) {
    const auto prefix = proxyPrefix();
    body = replaceAll(body, "href=\"/", "href=\"" + prefix + "/");
    body = replaceAll(body, "src=\"/", "src=\"" + prefix + "/");
    body = replaceAll(body, "action=\"/", "action=\"" + prefix + "/");
    body = replaceAll(body, "href='/", "href='" + prefix + "/");
    body = replaceAll(body, "src='/", "src='" + prefix + "/");
    body = replaceAll(body, "action='/", "action='" + prefix + "/");

    const auto headPos = body.find("<head");
    if (headPos != std::string::npos) {
        const auto tagEnd = body.find('>', headPos);
        if (tagEnd != std::string::npos) {
            body.insert(tagEnd + 1, "<base href=\"" + prefix + "/\">");
        }
    }
    return body;
}

std::optional<std::string> decodeChunkedBody(const std::string& input) {
    std::string output;
    size_t pos = 0;

    while (pos < input.size()) {
        size_t lineEnd = input.find("\r\n", pos);
        size_t lineSize = 2;
        if (lineEnd == std::string::npos) {
            lineEnd = input.find('\n', pos);
            lineSize = 1;
        }
        if (lineEnd == std::string::npos) {
            return std::nullopt;
        }

        auto lengthToken = trim(input.substr(pos, lineEnd - pos));
        const auto semi = lengthToken.find(';');
        if (semi != std::string::npos) {
            lengthToken = lengthToken.substr(0, semi);
        }
        if (lengthToken.empty()) {
            return std::nullopt;
        }

        char* end = nullptr;
        const auto chunkSize = std::strtoull(lengthToken.c_str(), &end, 16);
        if (end == nullptr || *end != '\0') {
            return std::nullopt;
        }

        pos = lineEnd + lineSize;
        if (chunkSize == 0) {
            return output;
        }
        if (pos + chunkSize > input.size()) {
            return std::nullopt;
        }

        output.append(input, pos, static_cast<size_t>(chunkSize));
        pos += static_cast<size_t>(chunkSize);

        if (input.compare(pos, 2, "\r\n") == 0) {
            pos += 2;
        } else if (input.compare(pos, 1, "\n") == 0) {
            pos += 1;
        } else {
            return std::nullopt;
        }
    }

    return std::nullopt;
}

std::optional<UpstreamResponse> fetchHttp(const std::string& host, int port, const std::string& requestText) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result) != 0) {
        return std::nullopt;
    }

    int sock = -1;
    for (addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock == -1) {
            continue;
        }
        if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }
        close(sock);
        sock = -1;
    }
    freeaddrinfo(result);

    if (sock == -1) {
        return std::nullopt;
    }

    ssize_t sentTotal = 0;
    while (sentTotal < static_cast<ssize_t>(requestText.size())) {
        const auto sent = send(sock, requestText.data() + sentTotal, requestText.size() - sentTotal, 0);
        if (sent <= 0) {
            close(sock);
            return std::nullopt;
        }
        sentTotal += sent;
    }

    std::string raw;
    std::array<char, 8192> buffer{};
    while (true) {
        const auto got = recv(sock, buffer.data(), buffer.size(), 0);
        if (got <= 0) {
            break;
        }
        raw.append(buffer.data(), static_cast<size_t>(got));
    }
    close(sock);

    const auto sep = raw.find("\r\n\r\n");
    const auto altSep = raw.find("\n\n");
    size_t headerEnd = std::string::npos;
    size_t headerSize = 0;
    if (sep != std::string::npos) {
        headerEnd = sep;
        headerSize = 4;
    } else if (altSep != std::string::npos) {
        headerEnd = altSep;
        headerSize = 2;
    } else {
        return std::nullopt;
    }

    UpstreamResponse response;
    std::string headerBlock = raw.substr(0, headerEnd);
    response.body = raw.substr(headerEnd + headerSize);

    std::stringstream headers(headerBlock);
    std::string line;
    if (!std::getline(headers, line)) {
        return std::nullopt;
    }
    line = trim(line);
    {
        std::smatch match;
        if (std::regex_search(line, match, std::regex(R"(HTTP/\d+\.\d+\s+(\d+)\s*(.*))"))) {
            response.status = std::stoi(match[1].str());
            response.reason = trim(match[2].str());
            if (response.reason.empty()) {
                response.reason = reasonPhrase(response.status);
            }
        }
    }

    while (std::getline(headers, line)) {
        line = trim(line);
        if (line.empty()) {
            continue;
        }
        const auto pos = line.find(':');
        if (pos == std::string::npos) {
            continue;
        }
        response.headers.emplace_back(trim(line.substr(0, pos)), trim(line.substr(pos + 1)));
    }
    return response;
}

std::string buildForwardRequest(const std::string& host, int port, const std::string& upstreamPath, const std::string& body,
                                const std::string& panelSessionId) {
    std::ostringstream request;
    const auto method = getenvOrEmpty("REQUEST_METHOD").empty() ? "GET" : getenvOrEmpty("REQUEST_METHOD");
    request << method << ' ' << upstreamPath << " HTTP/1.1\r\n";
    request << "Host: " << host << ':' << port << "\r\n";
    request << "Connection: close\r\n";
    request << "Accept-Encoding: identity\r\n";
    request << "X-Forwarded-Host: " << hostWithoutPort() << "\r\n";
    request << "X-Forwarded-Proto: " << requestScheme() << "\r\n";
    request << "X-Forwarded-Prefix: " << proxyPrefix() << "\r\n";

    for (char** env = environ; env != nullptr && *env != nullptr; ++env) {
        std::string entry(*env);
        if (entry.rfind("HTTP_", 0) != 0) {
            continue;
        }
        const auto eq = entry.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        auto name = entry.substr(5, eq - 5);
        auto value = entry.substr(eq + 1);
        std::replace(name.begin(), name.end(), '_', '-');
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        if (name == "cookie") {
            value = filterForwardCookies(value, panelSessionId);
            if (value.empty()) {
                continue;
            }
        }
        // Keep hop-by-hop headers and the panel-managed host header under proxy control.
        if (name == "host" || name == "connection" || name == "accept-encoding" || name == "content-length") {
            continue;
        }
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char ch) { return static_cast<char>(ch == '-' ? '-' : std::toupper(ch)); });
        request << name << ": " << value << "\r\n";
    }

    const auto contentType = getenvOrEmpty("CONTENT_TYPE");
    if (!contentType.empty()) {
        request << "Content-Type: " << contentType << "\r\n";
    }
    if (!body.empty()) {
        request << "Content-Length: " << body.size() << "\r\n";
    }
    request << "\r\n" << body;
    return request.str();
}

bool isHopByHop(const std::string& header) {
    const std::string lowered = [&header]() {
        std::string s = header;
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        return s;
    }();
    return lowered == "connection" || lowered == "transfer-encoding" || lowered == "content-length";
}

std::string renderStatusPage(const std::string& selectedApp, const std::string& infoJson, const std::string& message) {
    const auto nickname = findJsonString(infoJson, "nickname").value_or(selectedApp);
    const auto description = findJsonString(infoJson, "description").value_or("这个应用还没有提供描述。");
    const auto version = findJsonString(infoJson, "version").value_or("-");
    const auto running = findJsonBool(infoJson, "running").value_or(false);
    const auto desktopPort = findJsonInt(infoJson, "port");
    const auto hostNetwork = findJsonBool(infoJson, "host_network").value_or(false);

    std::ostringstream body;
    body << "<section class=\"card empty\">"
         << "<span class=\"badge\">" << (running ? "Running" : "Stopped") << "</span>"
         << "<h1>" << htmlEscape(nickname) << "</h1>"
         << "<p>" << htmlEscape(description) << "</p>"
         << "<p>" << htmlEscape(message) << "</p>"
         << "</section>"
         << "<section class=\"card\">"
         << "<div class=\"meta\">"
         << "<div><strong>应用名</strong>" << htmlEscape(selectedApp) << "</div>"
         << "<div><strong>版本</strong>" << htmlEscape(version) << "</div>"
         << "<div><strong>桌面端口</strong>" << (desktopPort.has_value() ? std::to_string(*desktopPort) : std::string("-")) << "</div>"
         << "<div><strong>访问方式</strong>" << (hostNetwork ? "Host Network" : "Container Proxy") << "</div>"
         << "</div>"
         << "<pre class=\"json\">" << htmlEscape(infoJson) << "</pre>"
         << "</section>";
    return htmlShell(nickname + " - Desktop", body.str());
}

}  // namespace

int main() {
    const auto session = currentSession();
    if (!session.has_value()) {
        sendHtml(401, renderEmpty("需要登录", "请先返回 CapOS 面板完成登录，然后再查看桌面应用。"));
        return 0;
    }

    const auto query = parseKv(getenvOrEmpty("QUERY_STRING"));
    std::string selectedApp;
    if (const auto it = query.find("app"); it != query.end()) {
        selectedApp = it->second;
    }

    if (selectedApp.empty()) {
        const auto desktop = runCapbox({"desktop", "get", "--user", session->username});
        if (desktop.exit_code != 0) {
            sendHtml(500, renderEmpty("桌面应用加载失败", trim(desktop.output)));
            return 0;
        }
        if (const auto app = findJsonString(desktop.output, "app"); app.has_value()) {
            selectedApp = *app;
        }
    }

    if (selectedApp.empty()) {
        sendHtml(200, renderEmpty("还没有桌面应用", "先在面板里从你已安装的应用中选择一个桌面应用，桌面区就会在这里显示。"));
        return 0;
    }

    const auto info = runCapbox({"app", "info", selectedApp, "--user", session->username});
    if (info.exit_code != 0) {
        sendHtml(404, renderEmpty("应用不存在", trim(info.output)));
        return 0;
    }

    const auto running = findJsonBool(info.output, "running").value_or(false);
    const auto scheme = findJsonString(info.output, "scheme").value_or("http");
    const auto host = findJsonString(info.output, "target_host");
    const auto port = findJsonInt(info.output, "port");

    if (!running || !host.has_value() || !port.has_value() || scheme != "http") {
        std::string why = "当前桌面应用还没有可代理的 HTTP 入口。";
        if (!running) {
            why = "当前桌面应用还没有启动。";
        } else if (!port.has_value()) {
            why = "当前桌面应用在元数据里没有声明 service 端口。";
        } else if (scheme != "http") {
            why = "当前桌面应用仅声明了 HTTPS 入口，这一轮代理先优先支持 HTTP。";
        }
        sendHtml(200, renderStatusPage(selectedApp, info.output, why));
        return 0;
    }

    auto path = effectivePathInfo();
    if (path.empty() || path == "/") {
        path = "/";
    }

    const auto upstreamQuery = joinQuery(query, {"app"});
    std::string upstreamPath = path;
    if (!upstreamQuery.empty()) {
        upstreamPath += (upstreamPath.find('?') == std::string::npos ? "?" : "&");
        upstreamPath += upstreamQuery;
    }

    const auto requestBody = readRequestBody();
    const auto requestText = buildForwardRequest(*host, static_cast<int>(*port), upstreamPath, requestBody, session->id);
    auto response = fetchHttp(*host, static_cast<int>(*port), requestText);
    if (!response.has_value()) {
        sendHtml(502, renderStatusPage(selectedApp, info.output, "连接桌面应用失败，可能容器尚未完全启动，或目标服务没有监听声明的端口。"));
        return 0;
    }

    std::string contentType;
    bool chunkedEncoding = false;
    for (const auto& [name, value] : response->headers) {
        std::string lowered = name;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        if (lowered == "content-type") {
            contentType = value;
        } else if (lowered == "transfer-encoding") {
            std::string loweredValue = value;
            std::transform(loweredValue.begin(), loweredValue.end(), loweredValue.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            if (loweredValue.find("chunked") != std::string::npos) {
                chunkedEncoding = true;
            }
        }
    }

    if (chunkedEncoding) {
        const auto decoded = decodeChunkedBody(response->body);
        if (!decoded.has_value()) {
            sendHtml(502, renderStatusPage(selectedApp, info.output, "桌面应用返回了无法解析的 chunked 响应，代理暂时无法继续。"));
            return 0;
        }
        response->body = *decoded;
    }

    if (contentType.find("text/html") != std::string::npos) {
        response->body = rewriteHtmlBody(response->body);
    }

    std::vector<std::pair<std::string, std::string>> outHeaders;
    for (const auto& [name, value] : response->headers) {
        if (isHopByHop(name)) {
            continue;
        }
        std::string lowered = name;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        if (lowered == "location" && !value.empty() && value[0] == '/') {
            outHeaders.emplace_back(name, proxyPrefix() + value);
        } else {
            outHeaders.emplace_back(name, value);
        }
    }
    outHeaders.emplace_back("Content-Length", std::to_string(response->body.size()));

    writeHeaders(response->status, contentType.empty() ? "text/plain; charset=utf-8" : contentType, outHeaders);
    std::cout << response->body;
    return 0;
}
