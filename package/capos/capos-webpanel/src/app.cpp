#include "common.hpp"

#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <poll.h>
#include <signal.h>
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

struct UpstreamConnection {
    int fd = -1;
    SSL_CTX* ctx = nullptr;
    SSL* ssl = nullptr;
    bool tls = false;

    UpstreamConnection() = default;
    UpstreamConnection(const UpstreamConnection&) = delete;
    UpstreamConnection& operator=(const UpstreamConnection&) = delete;

    UpstreamConnection(UpstreamConnection&& other) noexcept {
        *this = std::move(other);
    }

    UpstreamConnection& operator=(UpstreamConnection&& other) noexcept {
        if (this != &other) {
            close();
            fd = other.fd;
            ctx = other.ctx;
            ssl = other.ssl;
            tls = other.tls;
            other.fd = -1;
            other.ctx = nullptr;
            other.ssl = nullptr;
            other.tls = false;
        }
        return *this;
    }

    ~UpstreamConnection() {
        close();
    }

    void close() {
        if (ssl != nullptr) {
            SSL_shutdown(ssl);
            SSL_free(ssl);
            ssl = nullptr;
        }
        if (fd >= 0) {
            ::close(fd);
            fd = -1;
        }
        if (ctx != nullptr) {
            SSL_CTX_free(ctx);
            ctx = nullptr;
        }
        tls = false;
    }
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

std::string lowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

std::string requestScheme() {
    auto scheme = getenvOrEmpty("REQUEST_SCHEME");
    scheme = lowerAscii(scheme);
    if (scheme == "http" || scheme == "https") {
        return scheme;
    }

    auto https = getenvOrEmpty("HTTPS");
    https = lowerAscii(https);
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

bool isWebSocketRequest() {
    const auto upgrade = lowerAscii(getenvOrEmpty("HTTP_UPGRADE"));
    return upgrade == "websocket";
}

std::string stripQueryParam(const std::string& rawQuery, const std::string& excludedKey) {
    std::stringstream stream(rawQuery);
    std::string item;
    std::vector<std::string> kept;
    while (std::getline(stream, item, '&')) {
        if (item.empty()) {
            continue;
        }
        const auto eq = item.find('=');
        const auto key = urlDecode(item.substr(0, eq));
        if (key == excludedKey) {
            continue;
        }
        kept.push_back(item);
    }

    std::ostringstream out;
    for (size_t i = 0; i < kept.size(); ++i) {
        if (i != 0) {
            out << '&';
        }
        out << kept[i];
    }
    return out.str();
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
        << "--bg:#eef1f4;--panel:#fff;--ink:#18222c;--muted:#62717f;--line:#d7dde4;--accent:#276a8f;--danger:#b23b36;--soft:#edf5f8;}"
        << "html,body{height:100%;margin:0;font-family:'Segoe UI',sans-serif;background:linear-gradient(180deg,#f8fafb,#e8edf1);color:var(--ink);}body{display:flex;min-height:100%;}"
        << ".shell{display:flex;flex-direction:column;gap:14px;width:100%;padding:18px;box-sizing:border-box;}.card{background:rgba(255,255,255,.94);border:1px solid var(--line);border-radius:12px;box-shadow:0 16px 36px rgba(19,31,43,.12);}"
        << ".empty{padding:22px;display:flex;flex-direction:column;gap:10px;}.empty h1{margin:0;font-size:22px;}.empty p{margin:0;color:var(--muted);line-height:1.5;}"
        << ".meta{display:grid;grid-template-columns:repeat(auto-fit,minmax(160px,1fr));gap:10px;padding:0 22px 20px;}.meta div{background:#fbfcfd;border:1px solid var(--line);border-radius:8px;padding:10px 12px;overflow-wrap:anywhere;}"
        << ".meta strong{display:block;font-size:11px;text-transform:uppercase;color:var(--muted);margin-bottom:5px;}"
        << ".actions{display:flex;gap:8px;flex-wrap:wrap;padding:0 22px 20px;}.button{display:inline-flex;align-items:center;justify-content:center;border-radius:8px;padding:9px 12px;background:var(--accent);color:#fff;text-decoration:none;font-weight:700;border:0;cursor:pointer;font:inherit;}"
        << ".button.secondary{background:#fff;color:var(--ink);border:1px solid var(--line);}.button.danger{background:rgba(178,59,54,.1);color:var(--danger);}"
        << ".hint{padding:0 22px 20px;color:var(--muted);}.badge{display:inline-flex;align-items:center;padding:5px 9px;border-radius:999px;background:var(--soft);color:var(--accent);font-weight:700;font-size:12px;}"
        << ".json{margin:0 22px 22px;background:#18222c;color:#dce4eb;border-radius:8px;padding:14px;overflow:auto;font:12px/1.5 ui-monospace,monospace;}"
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
    const std::vector<std::string> markers = {
        "href=\"/", "src=\"/", "action=\"/", "poster=\"/",
        "href='/", "src='/", "action='/", "poster='/",
        "HREF=\"/", "SRC=\"/", "ACTION=\"/", "POSTER=\"/",
        "HREF='/", "SRC='/", "ACTION='/", "POSTER='/",
    };
    for (const auto& marker : markers) {
        size_t pos = 0;
        while ((pos = body.find(marker, pos)) != std::string::npos) {
            const auto slash = pos + marker.size() - 1;
            if (slash + 1 < body.size() && body[slash + 1] == '/') {
                pos = slash + 2;
                continue;
            }
            body.replace(slash, 1, prefix + "/");
            pos = slash + prefix.size() + 1;
        }
    }

    for (const auto& marker : {"url(/", "url('/", "url(\"/"}) {
        size_t pos = 0;
        while ((pos = body.find(marker, pos)) != std::string::npos) {
            const auto slash = pos + std::strlen(marker) - 1;
            if (slash + 1 < body.size() && body[slash + 1] == '/') {
                pos = slash + 2;
                continue;
            }
            body.replace(slash, 1, prefix + "/");
            pos = slash + prefix.size() + 1;
        }
    }

    const auto lowered = lowerAscii(body);
    const auto headPos = lowered.find("<head");
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

std::optional<int> connectTcp(const std::string& host, int port) {
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

    return sock;
}

std::optional<UpstreamConnection> openUpstream(const std::string& host, int port, const std::string& scheme, bool verifyTls) {
    auto sock = connectTcp(host, port);
    if (!sock.has_value()) {
        return std::nullopt;
    }

    UpstreamConnection connection;
    connection.fd = *sock;

    if (scheme == "https") {
        SSL_library_init();
        SSL_load_error_strings();
        connection.tls = true;
        connection.ctx = SSL_CTX_new(TLS_client_method());
        if (connection.ctx == nullptr) {
            return std::nullopt;
        }
        if (verifyTls) {
            SSL_CTX_set_verify(connection.ctx, SSL_VERIFY_PEER, nullptr);
            SSL_CTX_set_default_verify_paths(connection.ctx);
        } else {
            SSL_CTX_set_verify(connection.ctx, SSL_VERIFY_NONE, nullptr);
        }

        connection.ssl = SSL_new(connection.ctx);
        if (connection.ssl == nullptr) {
            return std::nullopt;
        }
        SSL_set_fd(connection.ssl, connection.fd);
        SSL_set_tlsext_host_name(connection.ssl, host.c_str());
        if (verifyTls) {
            X509_VERIFY_PARAM* param = SSL_get0_param(connection.ssl);
            X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
            X509_VERIFY_PARAM_set1_host(param, host.c_str(), 0);
        }
        if (SSL_connect(connection.ssl) != 1) {
            return std::nullopt;
        }
    }

    return connection;
}

bool writeUpstream(UpstreamConnection& connection, const char* data, size_t size) {
    size_t sentTotal = 0;
    while (sentTotal < size) {
        int sent = 0;
        if (connection.tls) {
            sent = SSL_write(connection.ssl, data + sentTotal, static_cast<int>(size - sentTotal));
            if (sent <= 0) {
                const auto err = SSL_get_error(connection.ssl, sent);
                if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                    continue;
                }
                return false;
            }
        } else {
            const auto rawSent = send(connection.fd, data + sentTotal, size - sentTotal, 0);
            if (rawSent <= 0) {
                return false;
            }
            sent = static_cast<int>(rawSent);
        }
        sentTotal += static_cast<size_t>(sent);
    }
    return true;
}

bool writeUpstream(UpstreamConnection& connection, const std::string& data) {
    return writeUpstream(connection, data.data(), data.size());
}

ssize_t readUpstream(UpstreamConnection& connection, char* buffer, size_t size) {
    if (connection.tls) {
        const auto got = SSL_read(connection.ssl, buffer, static_cast<int>(size));
        if (got <= 0) {
            const auto err = SSL_get_error(connection.ssl, got);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                return -2;
            }
            return 0;
        }
        return got;
    }
    return recv(connection.fd, buffer, size, 0);
}

struct HeaderReadResult {
    UpstreamResponse response;
    std::string firstBodyBytes;
};

std::optional<HeaderReadResult> readResponseHeaders(UpstreamConnection& connection) {
    std::string raw;
    std::array<char, 8192> buffer{};
    size_t headerEnd = std::string::npos;
    size_t headerSize = 0;

    while (headerEnd == std::string::npos) {
        const auto got = readUpstream(connection, buffer.data(), buffer.size());
        if (got <= 0) {
            return std::nullopt;
        }
        raw.append(buffer.data(), static_cast<size_t>(got));
        const auto sep = raw.find("\r\n\r\n");
        const auto altSep = raw.find("\n\n");
        if (sep != std::string::npos) {
            headerEnd = sep;
            headerSize = 4;
        } else if (altSep != std::string::npos) {
            headerEnd = altSep;
            headerSize = 2;
        }
    }

    HeaderReadResult parsed;
    std::string headerBlock = raw.substr(0, headerEnd);
    parsed.firstBodyBytes = raw.substr(headerEnd + headerSize);

    std::stringstream headers(headerBlock);
    std::string line;
    if (!std::getline(headers, line)) {
        return std::nullopt;
    }
    line = trim(line);
    {
        std::smatch match;
        if (std::regex_search(line, match, std::regex(R"(HTTP/\d+\.\d+\s+(\d+)\s*(.*))"))) {
            parsed.response.status = std::stoi(match[1].str());
            parsed.response.reason = trim(match[2].str());
            if (parsed.response.reason.empty()) {
                parsed.response.reason = reasonPhrase(parsed.response.status);
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
        parsed.response.headers.emplace_back(trim(line.substr(0, pos)), trim(line.substr(pos + 1)));
    }
    return parsed;
}

std::optional<UpstreamResponse> fetchHttp(const std::string& host, int port, const std::string& scheme, bool verifyTls,
                                          const std::string& requestText) {
    auto connection = openUpstream(host, port, scheme, verifyTls);
    if (!connection.has_value()) {
        return std::nullopt;
    }
    if (!writeUpstream(*connection, requestText)) {
        return std::nullopt;
    }

    auto parsed = readResponseHeaders(*connection);
    if (!parsed.has_value()) {
        return std::nullopt;
    }
    auto response = parsed->response;
    response.body = parsed->firstBodyBytes;

    std::array<char, 8192> buffer{};
    while (true) {
        const auto got = readUpstream(*connection, buffer.data(), buffer.size());
        if (got <= 0) {
            break;
        }
        response.body.append(buffer.data(), static_cast<size_t>(got));
    }

    return response;
}

bool isHopByHop(const std::string& header);

std::string buildForwardRequest(const std::string& host, int port, const std::string& upstreamPath, const std::string& body,
                                const std::string& panelSessionId, bool websocket = false) {
    std::ostringstream request;
    const auto method = getenvOrEmpty("REQUEST_METHOD").empty() ? "GET" : getenvOrEmpty("REQUEST_METHOD");
    request << method << ' ' << upstreamPath << " HTTP/1.1\r\n";
    request << "Host: " << host << ':' << port << "\r\n";
    request << "Connection: " << (websocket ? "Upgrade" : "close") << "\r\n";
    if (websocket) {
        request << "Upgrade: websocket\r\n";
    } else {
        request << "Accept-Encoding: identity\r\n";
    }
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
        name = lowerAscii(name);
        if (name == "cookie") {
            value = filterForwardCookies(value, panelSessionId);
            if (value.empty()) {
                continue;
            }
        }
        // Keep hop-by-hop headers and the panel-managed host header under proxy control.
        if (name == "host" || name == "accept-encoding" || name == "content-length" || isHopByHop(name)) {
            continue;
        }
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char ch) { return static_cast<char>(ch == '-' ? '-' : std::toupper(ch)); });
        request << name << ": " << value << "\r\n";
    }

    const auto contentType = getenvOrEmpty("CONTENT_TYPE");
    if (!contentType.empty()) {
        request << "Content-Type: " << contentType << "\r\n";
    }
    if (!body.empty() && !websocket) {
        request << "Content-Length: " << body.size() << "\r\n";
    }
    request << "\r\n";
    if (!websocket) {
        request << body;
    }
    return request.str();
}

bool isHopByHop(const std::string& header) {
    const auto lowered = lowerAscii(header);
    return lowered == "connection" ||
           lowered == "keep-alive" ||
           lowered == "proxy-authenticate" ||
           lowered == "proxy-authorization" ||
           lowered == "proxy-connection" ||
           lowered == "te" ||
           lowered == "trailer" ||
           lowered == "transfer-encoding" ||
           lowered == "upgrade" ||
           lowered == "content-length";
}

std::string rewriteResponseLocation(const std::string& value, const std::string& upstreamScheme,
                                    const std::string& upstreamHost, int upstreamPort) {
    if (value.empty()) {
        return value;
    }
    if (value[0] == '/' && (value.size() == 1 || value[1] != '/')) {
        return proxyPrefix() + value;
    }

    const auto prefixWithPort = upstreamScheme + "://" + upstreamHost + ":" + std::to_string(upstreamPort);
    if (value.rfind(prefixWithPort + "/", 0) == 0) {
        return proxyPrefix() + value.substr(prefixWithPort.size());
    }
    const auto prefixNoPort = upstreamScheme + "://" + upstreamHost;
    if (value.rfind(prefixNoPort + "/", 0) == 0) {
        return proxyPrefix() + value.substr(prefixNoPort.size());
    }
    return value;
}

std::string rewriteSetCookiePath(const std::string& value) {
    std::stringstream stream(value);
    std::string item;
    std::vector<std::string> parts;
    bool hasPath = false;
    while (std::getline(stream, item, ';')) {
        auto part = trim(item);
        if (part.empty()) {
            continue;
        }
        const auto lowered = lowerAscii(part);
        if (lowered.rfind("path=", 0) == 0) {
            parts.push_back("Path=" + proxyPrefix());
            hasPath = true;
        } else {
            parts.push_back(part);
        }
    }
    if (!hasPath) {
        parts.push_back("Path=" + proxyPrefix());
    }

    std::ostringstream out;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i != 0) {
            out << "; ";
        }
        out << parts[i];
    }
    return out.str();
}

bool writeStdoutAll(const char* data, size_t size) {
    size_t writtenTotal = 0;
    while (writtenTotal < size) {
        const auto written = ::write(STDOUT_FILENO, data + writtenTotal, size - writtenTotal);
        if (written <= 0) {
            return false;
        }
        writtenTotal += static_cast<size_t>(written);
    }
    return true;
}

bool writeStdoutAll(const std::string& data) {
    return writeStdoutAll(data.data(), data.size());
}

void writeWebSocketSwitchingHeaders(const UpstreamResponse& response) {
    bool hasUpgrade = false;
    bool hasConnection = false;
    std::cout << "Status: 101 Switching Protocols\r\n";
    for (const auto& [name, value] : response.headers) {
        const auto lowered = lowerAscii(name);
        if (lowered == "content-length" || lowered == "transfer-encoding" || lowered == "content-type") {
            continue;
        }
        if (lowered == "upgrade") {
            hasUpgrade = true;
        }
        if (lowered == "connection") {
            hasConnection = true;
        }
        std::cout << name << ": " << value << "\r\n";
    }
    if (!hasUpgrade) {
        std::cout << "Upgrade: websocket\r\n";
    }
    if (!hasConnection) {
        std::cout << "Connection: Upgrade\r\n";
    }
    std::cout << "\r\n";
    std::cout.flush();
}

void relayWebSocket(UpstreamConnection& connection, const std::string& initialBytes) {
    if (!initialBytes.empty()) {
        writeStdoutAll(initialBytes);
    }

    bool clientOpen = true;
    std::array<char, 8192> buffer{};
    while (true) {
        bool upstreamReady = connection.tls && SSL_pending(connection.ssl) > 0;
        pollfd fds[2]{};
        nfds_t nfds = 0;
        if (clientOpen) {
            fds[nfds].fd = STDIN_FILENO;
            fds[nfds].events = POLLIN;
            ++nfds;
        }
        fds[nfds].fd = connection.fd;
        fds[nfds].events = POLLIN;
        ++nfds;

        if (!upstreamReady) {
            const auto ready = poll(fds, nfds, -1);
            if (ready < 0) {
                if (errno == EINTR) {
                    continue;
                }
                break;
            }
        }

        size_t index = 0;
        if (clientOpen) {
            if ((fds[index].revents & (POLLIN | POLLHUP | POLLERR)) != 0) {
                const auto got = ::read(STDIN_FILENO, buffer.data(), buffer.size());
                if (got > 0) {
                    if (!writeUpstream(connection, buffer.data(), static_cast<size_t>(got))) {
                        break;
                    }
                } else {
                    clientOpen = false;
                }
            }
            ++index;
        }

        if (upstreamReady || (fds[index].revents & (POLLIN | POLLHUP | POLLERR)) != 0) {
            const auto got = readUpstream(connection, buffer.data(), buffer.size());
            if (got == -2) {
                continue;
            }
            if (got <= 0) {
                break;
            }
            if (!writeStdoutAll(buffer.data(), static_cast<size_t>(got))) {
                break;
            }
        }
    }
}

std::string renderStatusPage(const std::string& selectedApp, const std::string& infoJson, const std::string& message);

bool proxyWebSocket(const Session& session, const std::string& selectedApp, const std::string& infoJson,
                    const std::string& host, int port, const std::string& scheme, bool verifyTls,
                    const std::string& upstreamPath) {
    auto connection = openUpstream(host, port, scheme, verifyTls);
    if (!connection.has_value()) {
        sendHtml(502, renderStatusPage(selectedApp, infoJson, "WebSocket 连接上游失败，目标服务可能尚未监听。"));
        return false;
    }

    const auto requestText = buildForwardRequest(host, port, upstreamPath, "", session.id, true);
    if (!writeUpstream(*connection, requestText)) {
        sendHtml(502, renderStatusPage(selectedApp, infoJson, "WebSocket 握手请求发送失败。"));
        return false;
    }

    auto parsed = readResponseHeaders(*connection);
    if (!parsed.has_value()) {
        sendHtml(502, renderStatusPage(selectedApp, infoJson, "WebSocket 上游没有返回有效握手响应。"));
        return false;
    }
    if (parsed->response.status != 101) {
        sendHtml(502, renderStatusPage(selectedApp, infoJson, "WebSocket 上游拒绝升级连接，未返回 101 Switching Protocols。"));
        return false;
    }

    writeWebSocketSwitchingHeaders(parsed->response);
    relayWebSocket(*connection, parsed->firstBodyBytes);
    return true;
}

std::string renderStatusPage(const std::string& selectedApp, const std::string& infoJson, const std::string& message) {
    const auto nickname = findJsonString(infoJson, "nickname").value_or(selectedApp);
    const auto description = findJsonString(infoJson, "description").value_or("这个应用还没有提供描述。");
    const auto version = findJsonString(infoJson, "version").value_or("-");
    const auto running = findJsonBool(infoJson, "running").value_or(false);
    const auto desktopPort = findJsonInt(infoJson, "port");
    const auto hostNetwork = findJsonBool(infoJson, "host_network").value_or(false);
    const auto targetHost = findJsonString(infoJson, "target_host").value_or("-");
    const auto scheme = findJsonString(infoJson, "scheme").value_or("-");
    const auto target = desktopPort.has_value() ? scheme + "://" + targetHost + ":" + std::to_string(*desktopPort) : std::string("-");
    const auto appUrl = urlEncode(selectedApp);

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
         << "<div><strong>目标</strong>" << htmlEscape(target) << "</div>"
         << "</div>"
         << "<div class=\"actions\">"
         << "<button class=\"button\" onclick=\"fetch('/cgi-bin/cap/api/apps/" << htmlEscape(appUrl) << "/start',{method:'POST',credentials:'same-origin'}).then(function(){location.reload();})\">启动应用</button>"
         << "<a class=\"button secondary\" href=\"javascript:location.reload()\">刷新</a>"
         << "<a class=\"button secondary\" target=\"_top\" href=\"/cap/\">返回面板</a>"
         << "</div>"
         << "<pre class=\"json\">" << htmlEscape(infoJson) << "</pre>"
         << "</section>";
    return htmlShell(nickname + " - Desktop", body.str());
}

}  // namespace

int main() {
    signal(SIGPIPE, SIG_IGN);

    const auto session = currentSession();
    if (!session.has_value()) {
        sendHtml(401, renderEmpty("需要登录", "请先返回 CapOS 面板完成登录，然后再查看桌面应用。"));
        return 0;
    }

    const auto rawQuery = getenvOrEmpty("QUERY_STRING");
    const auto query = parseKv(rawQuery);
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
    const auto httpsSkipCheck = findJsonBool(info.output, "https_skip_check").value_or(true);

    if (!running || !host.has_value() || !port.has_value() || (scheme != "http" && scheme != "https")) {
        std::string why = "当前桌面应用还没有可代理的 HTTP/HTTPS 入口。";
        if (!running) {
            why = "当前桌面应用还没有启动。";
        } else if (!port.has_value()) {
            why = "当前桌面应用在元数据里没有声明 service 端口。";
        } else if (scheme != "http" && scheme != "https") {
            why = "当前桌面应用声明了暂不支持的 service scheme。";
        }
        sendHtml(200, renderStatusPage(selectedApp, info.output, why));
        return 0;
    }

    auto path = effectivePathInfo();
    if (path.empty() || path == "/") {
        path = "/";
    }

    const auto upstreamQuery = stripQueryParam(rawQuery, "app");
    std::string upstreamPath = path;
    if (!upstreamQuery.empty()) {
        upstreamPath += (upstreamPath.find('?') == std::string::npos ? "?" : "&");
        upstreamPath += upstreamQuery;
    }

    if (isWebSocketRequest()) {
        proxyWebSocket(*session, selectedApp, info.output, *host, static_cast<int>(*port), scheme, !httpsSkipCheck, upstreamPath);
        return 0;
    }

    const auto requestBody = readRequestBody();
    const auto requestText = buildForwardRequest(*host, static_cast<int>(*port), upstreamPath, requestBody, session->id);
    auto response = fetchHttp(*host, static_cast<int>(*port), scheme, !httpsSkipCheck, requestText);
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

    if (lowerAscii(contentType).find("text/html") != std::string::npos) {
        response->body = rewriteHtmlBody(response->body);
    }

    std::vector<std::pair<std::string, std::string>> outHeaders;
    for (const auto& [name, value] : response->headers) {
        std::string lowered = name;
        lowered = lowerAscii(lowered);
        if (isHopByHop(lowered) || lowered == "content-type") {
            continue;
        }
        if (lowered == "location") {
            outHeaders.emplace_back(name, rewriteResponseLocation(value, scheme, *host, static_cast<int>(*port)));
        } else if (lowered == "set-cookie") {
            outHeaders.emplace_back(name, rewriteSetCookiePath(value));
        } else {
            outHeaders.emplace_back(name, value);
        }
    }
    outHeaders.emplace_back("Content-Length", std::to_string(response->body.size()));

    writeHeaders(response->status, contentType.empty() ? "text/plain; charset=utf-8" : contentType, outHeaders);
    std::cout << response->body;
    return 0;
}
