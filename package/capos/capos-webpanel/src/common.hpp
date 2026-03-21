#ifndef CAPOS_WEBPANEL_COMMON_HPP
#define CAPOS_WEBPANEL_COMMON_HPP

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <pwd.h>
#include <regex>
#include <random>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

#include <grp.h>
#include <shadow.h>

#if __has_include(<crypt.h>)
#include <crypt.h>
#endif

namespace capos {

struct ExecResult {
    int exit_code = 0;
    std::string output;
};

struct Session {
    std::string id;
    std::string username;
    uid_t uid = 0;
    bool is_sudo = false;
    std::time_t created_at = 0;
    std::time_t expires_at = 0;
};

inline constexpr const char* kSessionCookieName = "capos_session";
inline constexpr const char* kSessionDir = "/tmp/capos-webpanel/sessions";
inline constexpr const char* kUploadDir = "/tmp/capos-webpanel/uploads";
inline constexpr std::time_t kSessionTtl = 60 * 60 * 12;

inline std::string getenvOrEmpty(const char* key) {
    const char* value = std::getenv(key);
    return value == nullptr ? std::string() : std::string(value);
}

inline std::string trim(const std::string& value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

inline std::string urlDecode(const std::string& input) {
    std::ostringstream decoded;
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '%' && i + 2 < input.size()) {
            const std::string hex = input.substr(i + 1, 2);
            char* end = nullptr;
            const auto value = static_cast<char>(std::strtol(hex.c_str(), &end, 16));
            if (end != nullptr && *end == '\0') {
                decoded << value;
                i += 2;
                continue;
            }
        }
        if (input[i] == '+') {
            decoded << ' ';
        } else {
            decoded << input[i];
        }
    }
    return decoded.str();
}

inline std::string jsonEscape(const std::string& input) {
    std::ostringstream out;
    for (const char ch : input) {
        switch (ch) {
            case '\\': out << "\\\\"; break;
            case '"': out << "\\\""; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20) {
                    out << "\\u"
                        << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<int>(static_cast<unsigned char>(ch))
                        << std::dec << std::setfill(' ');
                } else {
                    out << ch;
                }
        }
    }
    return out.str();
}

inline std::map<std::string, std::string> parseKv(const std::string& input) {
    std::map<std::string, std::string> values;
    std::stringstream stream(input);
    std::string item;
    while (std::getline(stream, item, '&')) {
        if (item.empty()) {
            continue;
        }
        const auto pos = item.find('=');
        const auto key = urlDecode(item.substr(0, pos));
        const auto value = pos == std::string::npos ? "" : urlDecode(item.substr(pos + 1));
        values[key] = value;
    }
    return values;
}

inline std::map<std::string, std::string> parseCookies(const std::string& cookieHeader) {
    std::map<std::string, std::string> cookies;
    std::stringstream stream(cookieHeader);
    std::string item;
    while (std::getline(stream, item, ';')) {
        const auto pos = item.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        const auto key = trim(item.substr(0, pos));
        const auto value = trim(item.substr(pos + 1));
        if (!key.empty()) {
            cookies[key] = value;
        }
    }
    return cookies;
}

inline std::vector<std::string> splitPath(const std::string& path) {
    std::vector<std::string> segments;
    std::stringstream stream(path);
    std::string item;
    while (std::getline(stream, item, '/')) {
        if (!item.empty()) {
            segments.push_back(item);
        }
    }
    return segments;
}

inline std::string readRequestBody() {
    const auto contentLength = getenvOrEmpty("CONTENT_LENGTH");
    std::string body;
    if (!contentLength.empty()) {
        const auto length = static_cast<size_t>(std::strtoull(contentLength.c_str(), nullptr, 10));
        body.resize(length);
        std::cin.read(body.data(), static_cast<std::streamsize>(length));
        body.resize(static_cast<size_t>(std::cin.gcount()));
        return body;
    }

    std::ostringstream out;
    out << std::cin.rdbuf();
    return out.str();
}

inline bool ensureDir(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    if (::mkdir(path.c_str(), 0700) == 0) {
        return true;
    }
    if (errno == EEXIST) {
        return true;
    }

    const auto slash = path.find_last_of('/');
    if (slash == std::string::npos || slash == 0) {
        return false;
    }

    if (!ensureDir(path.substr(0, slash))) {
        return false;
    }
    if (::mkdir(path.c_str(), 0700) == 0 || errno == EEXIST) {
        return true;
    }
    return false;
}

inline bool writeFile(const std::string& path, const std::string& content) {
    const auto slash = path.find_last_of('/');
    if (slash != std::string::npos) {
        ensureDir(path.substr(0, slash));
    }
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }
    out << content;
    return static_cast<bool>(out);
}

inline std::optional<std::string> readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return std::nullopt;
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

inline std::string shellQuote(const std::string& input) {
    std::string quoted = "'";
    for (const char ch : input) {
        if (ch == '\'') {
            quoted += "'\\''";
        } else {
            quoted += ch;
        }
    }
    quoted += "'";
    return quoted;
}

inline ExecResult execCommand(const std::string& command) {
    ExecResult result;
    std::array<char, 4096> buffer{};
    FILE* pipe = popen((command + " 2>&1").c_str(), "r");
    if (pipe == nullptr) {
        result.exit_code = 127;
        result.output = "failed to execute command";
        return result;
    }

    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        result.output += buffer.data();
    }

    const int status = pclose(pipe);
    if (status == -1) {
        result.exit_code = 127;
    } else {
        result.exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : status;
    }
    return result;
}

inline std::string randomHex(size_t bytes) {
    std::string data(bytes, '\0');
    int fd = ::open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        const auto got = ::read(fd, data.data(), data.size());
        ::close(fd);
        if (got < 0) {
            data.assign(bytes, '\0');
        } else if (static_cast<size_t>(got) < bytes) {
            data.resize(static_cast<size_t>(got));
        }
    } else {
        data.clear();
    }

    if (data.empty()) {
        std::random_device rd;
        data.resize(bytes);
        for (size_t i = 0; i < bytes; ++i) {
            data[i] = static_cast<char>(rd() & 0xff);
        }
    }

    std::ostringstream hex;
    for (unsigned char ch : data) {
        hex << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(ch);
    }
    return hex.str();
}

inline bool userIsSudo(const std::string& username) {
    if (username == "root") {
        return true;
    }

    const auto sudoResult = execCommand("command -v sudo >/dev/null 2>&1 && sudo -l -U " + shellQuote(username));
    if (sudoResult.exit_code == 0) {
        return true;
    }

    const auto groupResult = execCommand("id -Gn " + shellQuote(username));
    if (groupResult.exit_code != 0) {
        return false;
    }

    const auto groups = " " + trim(groupResult.output) + " ";
    return groups.find(" sudo ") != std::string::npos || groups.find(" wheel ") != std::string::npos;
}

inline bool verifyPassword(const std::string& username, const std::string& password, uid_t& uidOut, std::string& error) {
    errno = 0;
    passwd* pw = getpwnam(username.c_str());
    if (pw == nullptr) {
        error = "invalid username or password";
        return false;
    }

    std::string hash = pw->pw_passwd == nullptr ? "" : std::string(pw->pw_passwd);
    if (hash == "x" || hash == "*" || hash == "!") {
        errno = 0;
        spwd* sp = getspnam(username.c_str());
        if (sp != nullptr && sp->sp_pwdp != nullptr) {
            hash = sp->sp_pwdp;
        }
    }

    if (hash.empty() || hash == "!" || hash == "*" || hash == "x") {
        error = "invalid username or password";
        return false;
    }

    char* encrypted = ::crypt(password.c_str(), hash.c_str());
    if (encrypted == nullptr) {
        error = "password verification failed";
        return false;
    }
    if (hash != encrypted) {
        error = "invalid username or password";
        return false;
    }

    uidOut = pw->pw_uid;
    return true;
}

inline std::string sessionPath(const std::string& sessionId) {
    return std::string(kSessionDir) + "/" + sessionId + ".session";
}

inline std::string sessionCookieHeader(const std::string& value, std::time_t maxAge) {
    std::ostringstream header;
    header << kSessionCookieName << '=' << value
           << "; Path=/; HttpOnly; SameSite=Lax; Max-Age=" << maxAge;
    return header.str();
}

inline std::string uploadPathFor(const std::string& sessionId, const std::string& uploadId, const std::string& name) {
    return std::string(kUploadDir) + "/" + sessionId + "/" + uploadId + "_" + name;
}

inline bool saveSession(const Session& session) {
    std::ostringstream data;
    data << "id=" << session.id << "\n"
         << "username=" << session.username << "\n"
         << "uid=" << session.uid << "\n"
         << "is_sudo=" << (session.is_sudo ? "1" : "0") << "\n"
         << "created_at=" << session.created_at << "\n"
         << "expires_at=" << session.expires_at << "\n";
    return writeFile(sessionPath(session.id), data.str());
}

inline std::optional<Session> loadSessionById(const std::string& sessionId) {
    const auto content = readFile(sessionPath(sessionId));
    if (!content.has_value()) {
        return std::nullopt;
    }

    std::map<std::string, std::string> values;
    std::stringstream stream(*content);
    std::string line;
    while (std::getline(stream, line)) {
        const auto pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        values[line.substr(0, pos)] = line.substr(pos + 1);
    }

    Session session;
    session.id = values["id"];
    session.username = values["username"];
    session.uid = static_cast<uid_t>(std::strtoul(values["uid"].c_str(), nullptr, 10));
    session.is_sudo = values["is_sudo"] == "1";
    session.created_at = static_cast<std::time_t>(std::strtoll(values["created_at"].c_str(), nullptr, 10));
    session.expires_at = static_cast<std::time_t>(std::strtoll(values["expires_at"].c_str(), nullptr, 10));
    if (session.id.empty() || session.username.empty()) {
        return std::nullopt;
    }
    if (session.expires_at < std::time(nullptr)) {
        std::remove(sessionPath(session.id).c_str());
        return std::nullopt;
    }
    return session;
}

inline std::optional<Session> currentSession() {
    const auto cookies = parseCookies(getenvOrEmpty("HTTP_COOKIE"));
    const auto it = cookies.find(kSessionCookieName);
    if (it == cookies.end()) {
        return std::nullopt;
    }
    return loadSessionById(it->second);
}

inline void deleteSession(const std::string& sessionId) {
    std::remove(sessionPath(sessionId).c_str());
}

inline std::string reasonPhrase(int status) {
    switch (status) {
        case 200: return "OK";
        case 201: return "Created";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 409: return "Conflict";
        case 500: return "Internal Server Error";
        default: return "OK";
    }
}

inline void writeHeaders(int status, const std::string& contentType, const std::vector<std::pair<std::string, std::string>>& extraHeaders = {}) {
    std::cout << "Status: " << status << ' ' << reasonPhrase(status) << "\r\n";
    std::cout << "Content-Type: " << contentType << "\r\n";
    std::cout << "Cache-Control: no-store\r\n";
    for (const auto& [name, value] : extraHeaders) {
        std::cout << name << ": " << value << "\r\n";
    }
    std::cout << "\r\n";
}

inline void sendJson(int status, const std::string& body, const std::vector<std::pair<std::string, std::string>>& extraHeaders = {}) {
    writeHeaders(status, "application/json; charset=utf-8", extraHeaders);
    std::cout << body;
}

inline void sendHtml(int status, const std::string& body, const std::vector<std::pair<std::string, std::string>>& extraHeaders = {}) {
    writeHeaders(status, "text/html; charset=utf-8", extraHeaders);
    std::cout << body;
}

inline void sendRedirect(const std::string& location, const std::vector<std::pair<std::string, std::string>>& extraHeaders = {}) {
    auto headers = extraHeaders;
    headers.emplace_back("Location", location);
    writeHeaders(302, "text/plain; charset=utf-8", headers);
}

inline std::string jsonError(const std::string& message, const std::string& code = "ERROR") {
    return "{\"ok\":false,\"error_code\":\"" + jsonEscape(code) + "\",\"message\":\"" + jsonEscape(message) + "\"}";
}

inline std::string jsonOk(const std::string& dataJson) {
    return "{\"ok\":true,\"data\":" + dataJson + "}";
}

inline std::string sanitizeUploadFilename(std::string filename) {
    for (char& ch : filename) {
        const bool safe =
            (ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') ||
            ch == '.' || ch == '_' || ch == '-';
        if (!safe) {
            ch = '_';
        }
    }
    if (filename.empty()) {
        filename = "upload.cpk";
    }
    return filename;
}

inline ExecResult runCapbox(const std::vector<std::string>& args) {
    std::ostringstream command;
    command << "capbox";
    for (const auto& arg : args) {
        command << ' ' << shellQuote(arg);
    }
    return execCommand(command.str());
}

inline std::optional<std::string> regexFirst(const std::string& text, const std::regex& pattern) {
    std::smatch match;
    if (std::regex_search(text, match, pattern) && match.size() > 1) {
        return match[1].str();
    }
    return std::nullopt;
}

inline std::optional<std::string> findJsonString(const std::string& json, const std::string& key) {
    return regexFirst(json, std::regex("\"" + key + "\"\\s*:\\s*\"([^\"]*)\""));
}

inline std::optional<long long> findJsonInt(const std::string& json, const std::string& key) {
    const auto match = regexFirst(json, std::regex("\"" + key + "\"\\s*:\\s*([0-9]+)"));
    if (!match.has_value()) {
        return std::nullopt;
    }
    return std::strtoll(match->c_str(), nullptr, 10);
}

inline std::optional<bool> findJsonBool(const std::string& json, const std::string& key) {
    const auto match = regexFirst(json, std::regex("\"" + key + "\"\\s*:\\s*(true|false)"));
    if (!match.has_value()) {
        return std::nullopt;
    }
    return *match == "true";
}

inline std::string hostWithoutPort() {
    auto host = getenvOrEmpty("HTTP_HOST");
    if (host.empty()) {
        host = "localhost";
    }
    const auto pos = host.find(':');
    if (pos != std::string::npos) {
        host = host.substr(0, pos);
    }
    return host;
}

inline std::string htmlEscape(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (const char ch : input) {
        switch (ch) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default: out += ch; break;
        }
    }
    return out;
}

}  // namespace capos

#endif
