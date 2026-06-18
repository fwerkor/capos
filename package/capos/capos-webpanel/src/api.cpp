#include "common.hpp"

#include <set>

using namespace capos;

namespace {

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
    const std::string prefix = "/cgi-bin/cap/api";
    if (uri.rfind(prefix, 0) == 0) {
        return uri.substr(prefix.size());
    }
    return "";
}

std::string sessionJson(const Session& session) {
    std::ostringstream out;
    out << "{"
        << "\"authenticated\":true,"
        << "\"username\":\"" << jsonEscape(session.username) << "\","
        << "\"uid\":" << static_cast<unsigned long>(session.uid) << ","
        << "\"is_sudo\":" << (session.is_sudo ? "true" : "false")
        << "}";
    return out.str();
}

bool isValidAppName(const std::string& app) {
    if (app.empty() || app.size() > 64) {
        return false;
    }
    return std::all_of(app.begin(), app.end(), [](unsigned char ch) {
        return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_';
    });
}

bool isValidUsername(const std::string& username) {
    if (username.empty() || username.size() > 64) {
        return false;
    }
    return std::all_of(username.begin(), username.end(), [](unsigned char ch) {
        return std::isalnum(ch) != 0 || ch == '_' || ch == '-' || ch == '.';
    });
}

bool validateAppOrSend(const std::string& app) {
    if (isValidAppName(app)) {
        return true;
    }
    sendJson(400, jsonError("invalid app name", "INVALID_APP_NAME"));
    return false;
}

bool parsePortValue(const std::string& value, std::string& normalized) {
    if (value.empty() || value.size() > 5) {
        return false;
    }
    if (!std::all_of(value.begin(), value.end(), [](unsigned char ch) { return std::isdigit(ch) != 0; })) {
        return false;
    }
    const auto port = std::strtol(value.c_str(), nullptr, 10);
    if (port < 1 || port > 65535) {
        return false;
    }
    normalized = std::to_string(port);
    return true;
}

bool normalizeProto(const std::string& value, std::string& proto) {
    proto = value.empty() ? "tcp" : value;
    std::transform(proto.begin(), proto.end(), proto.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return proto == "tcp" || proto == "udp";
}

bool isValidUploadId(const std::string& uploadId) {
    return isHexToken(uploadId, 16, 16);
}

bool hasSuffix(const std::string& value, const std::string& suffix) {
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool isValidUploadFilename(const std::string& filename) {
    if (filename.empty() || filename.size() > 120 || filename == "." || filename == "..") {
        return false;
    }
    if (filename.find('/') != std::string::npos || filename.find('\\') != std::string::npos) {
        return false;
    }
    if (!std::all_of(filename.begin(), filename.end(), [](unsigned char ch) {
            return std::isalnum(ch) != 0 || ch == '.' || ch == '_' || ch == '-';
        })) {
        return false;
    }
    return filename.size() >= 4 &&
           (hasSuffix(filename, ".cpk") || hasSuffix(filename, ".tgz") || hasSuffix(filename, ".tar.gz"));
}

bool isAllowedAppAction(const std::string& action) {
    static const std::set<std::string> allowed = {"start", "stop", "restart", "uninstall"};
    return allowed.count(action) != 0;
}

int capboxFailureStatus(const std::string& output, int fallback = 400) {
    const auto lowered = [&output]() {
        std::string value = output;
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        return value;
    }();
    if (lowered.find("already installed") != std::string::npos ||
        lowered.find("already allocated") != std::string::npos ||
        lowered.find("already in use") != std::string::npos) {
        return 409;
    }
    if (lowered.find("sudo") != std::string::npos ||
        lowered.find("permission") != std::string::npos ||
        lowered.find("not enabled") != std::string::npos ||
        lowered.find("not allowed") != std::string::npos) {
        return 403;
    }
    if (lowered.find("not installed") != std::string::npos ||
        lowered.find("not found") != std::string::npos) {
        return 404;
    }
    return fallback;
}

std::optional<Session> requireSession() {
    const auto session = currentSession();
    if (!session.has_value()) {
        sendJson(401, jsonError("authentication required", "UNAUTHENTICATED"));
        return std::nullopt;
    }
    return session;
}

std::string uploadMetaPath(const std::string& sessionId, const std::string& uploadId) {
    return std::string(kUploadDir) + "/" + sessionId + "/" + uploadId + ".meta";
}

std::optional<std::string> lookupUploadPath(const std::string& sessionId, const std::string& uploadId) {
    const auto content = readFile(uploadMetaPath(sessionId, uploadId));
    if (!content.has_value()) {
        return std::nullopt;
    }
    return trim(*content);
}

std::vector<std::string> splitBodyLines(const std::string& body) {
    std::vector<std::string> lines;
    std::stringstream stream(body);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    return lines;
}

void handleRoot() {
    sendJson(200, "{\"ok\":true,\"service\":\"capos-webpanel-api\"}");
}

void handleHostexec() {
    if (getenvOrEmpty("REQUEST_METHOD") != "POST") {
        sendJson(405, jsonError("method not allowed", "METHOD_NOT_ALLOWED"));
        return;
    }

    const auto app = trim(getenvOrEmpty("HTTP_X_CAPOS_APP"));
    const auto user = trim(getenvOrEmpty("HTTP_X_CAPOS_USER"));
    const auto token = trim(getenvOrEmpty("HTTP_X_CAPOS_TOKEN"));
    if (app.empty() || user.empty() || token.empty()) {
        sendJson(400, jsonError("X-CapOS-App, X-CapOS-User and X-CapOS-Token are required", "INVALID_REQUEST"));
        return;
    }
    if (!isValidAppName(app) || !isValidUsername(user) || !isHexToken(token, 32, 128)) {
        sendJson(400, jsonError("invalid hostexec headers", "INVALID_REQUEST"));
        return;
    }

    const auto lines = splitBodyLines(readRequestBody());
    if (lines.empty() || trim(lines.front()).empty()) {
        sendJson(400, jsonError("request body must contain command path on the first line", "INVALID_REQUEST"));
        return;
    }

    std::vector<std::string> args = {"hostexec", "invoke", app, "--user", user, "--token", token, "--"};
    for (const auto& line : lines) {
        args.push_back(line);
    }

    const auto result = runCapbox(args);
    if (result.exit_code != 0) {
        sendJson(403, jsonError(trim(result.output), "HOSTEXEC_DENIED"));
        return;
    }
    sendJson(200, trim(result.output));
}

void handleLogin() {
    if (getenvOrEmpty("REQUEST_METHOD") != "POST") {
        sendJson(405, jsonError("method not allowed", "METHOD_NOT_ALLOWED"));
        return;
    }

    const auto form = parseKv(readRequestBody());
    const auto usernameIt = form.find("username");
    const auto passwordIt = form.find("password");
    if (usernameIt == form.end() || passwordIt == form.end()) {
        sendJson(400, jsonError("username and password are required", "INVALID_REQUEST"));
        return;
    }

    uid_t uid = 0;
    std::string error;
    if (!verifyPassword(usernameIt->second, passwordIt->second, uid, error)) {
        sendJson(401, jsonError(error, "INVALID_CREDENTIALS"));
        return;
    }

    Session session;
    session.id = randomHex(24);
    session.username = usernameIt->second;
    session.uid = uid;
    session.is_sudo = userIsSudo(session.username);
    session.created_at = std::time(nullptr);
    session.expires_at = session.created_at + kSessionTtl;

    if (!saveSession(session)) {
        sendJson(500, jsonError("failed to persist session"));
        return;
    }
    cleanupExpiredSessions();

    sendJson(
        200,
        "{\"ok\":true,\"session\":" + sessionJson(session) + "}",
        {{"Set-Cookie", sessionCookieHeader(session.id, kSessionTtl)}}
    );
}

void handleLogout() {
    if (const auto session = currentSession(); session.has_value()) {
        deleteSession(session->id);
    }
    sendJson(
        200,
        "{\"ok\":true}",
        {{"Set-Cookie", sessionCookieHeader("", 0)}}
    );
}

void handleSessionInfo() {
    const auto session = currentSession();
    if (!session.has_value()) {
        sendJson(200, "{\"ok\":true,\"authenticated\":false}");
        return;
    }
    sendJson(200, "{\"ok\":true,\"authenticated\":true,\"session\":" + sessionJson(*session) + "}");
}

void handleMe(const Session& session) {
    sendJson(200, "{\"ok\":true,\"session\":" + sessionJson(session) + "}");
}

void handleSystem(const Session& session) {
    const auto result = runCapbox({"user", "info", "--user", session.username});
    if (result.exit_code != 0) {
        sendJson(500, jsonError(trim(result.output), "SYSTEM_INFO_FAILED"));
        return;
    }
    sendJson(200, "{\"ok\":true,\"service\":\"capos-webpanel\",\"user\":" + trim(result.output) + "}");
}

void handleAppsList(const Session& session) {
    const auto result = runCapbox({"app", "list", "--user", session.username});
    if (result.exit_code != 0) {
        sendJson(500, jsonError(trim(result.output)));
        return;
    }
    sendJson(200, "{\"ok\":true,\"apps\":" + trim(result.output) + "}");
}

void handleAppInfo(const Session& session, const std::string& app) {
    if (!validateAppOrSend(app)) {
        return;
    }
    const auto result = runCapbox({"app", "info", app, "--user", session.username});
    if (result.exit_code != 0) {
        sendJson(404, jsonError(trim(result.output), "APP_NOT_FOUND"));
        return;
    }
    sendJson(200, trim(result.output));
}

void handleAppAction(const Session& session, const std::string& app, const std::string& action) {
    if (getenvOrEmpty("REQUEST_METHOD") != "POST") {
        sendJson(405, jsonError("method not allowed", "METHOD_NOT_ALLOWED"));
        return;
    }
    if (!validateAppOrSend(app)) {
        return;
    }
    if (!isAllowedAppAction(action)) {
        sendJson(400, jsonError("unsupported app action", "INVALID_ACTION"));
        return;
    }

    const auto result = runCapbox({"app", action, app, "--user", session.username});
    if (result.exit_code != 0) {
        const auto output = trim(result.output);
        sendJson(capboxFailureStatus(output), jsonError(output, "APP_ACTION_FAILED"));
        return;
    }
    sendJson(200, trim(result.output));
}

void handleAppReconcile(const Session& session, const std::string& app) {
    if (getenvOrEmpty("REQUEST_METHOD") != "POST") {
        sendJson(405, jsonError("method not allowed", "METHOD_NOT_ALLOWED"));
        return;
    }
    if (!validateAppOrSend(app)) {
        return;
    }
    const auto result = runCapbox({"reconcile", app, "--user", session.username});
    if (result.exit_code != 0) {
        const auto output = trim(result.output);
        sendJson(capboxFailureStatus(output), jsonError(output, "RECONCILE_FAILED"));
        return;
    }
    sendJson(200, trim(result.output));
}

void handleAppLogs(const Session& session, const std::string& app) {
    if (getenvOrEmpty("REQUEST_METHOD") != "GET") {
        sendJson(405, jsonError("method not allowed", "METHOD_NOT_ALLOWED"));
        return;
    }
    if (!validateAppOrSend(app)) {
        return;
    }
    const auto result = runCapbox({"app", "logs", app, "--user", session.username});
    if (result.exit_code != 0) {
        const auto output = trim(result.output);
        sendJson(capboxFailureStatus(output), jsonError(output, "APP_LOGS_FAILED"));
        return;
    }
    sendJson(200, trim(result.output));
}

void handleAppPorts(const Session& session, const std::string& app) {
    if (!validateAppOrSend(app)) {
        return;
    }
    const auto method = getenvOrEmpty("REQUEST_METHOD");
    if (method == "GET") {
        const auto result = runCapbox({"portmap", "list", app, "--user", session.username});
        if (result.exit_code != 0) {
            sendJson(400, jsonError(trim(result.output), "PORTMAP_LIST_FAILED"));
            return;
        }
        sendJson(200, "{\"ok\":true,\"portmaps\":" + trim(result.output) + "}");
        return;
    }

    if (method != "POST") {
        sendJson(405, jsonError("method not allowed", "METHOD_NOT_ALLOWED"));
        return;
    }

    const auto form = parseKv(readRequestBody());
    const auto action = form.count("action") ? form.at("action") : "set";
    const auto listenIt = form.find("listen");
    const auto targetIt = form.find("target");
    std::string proto;
    if (listenIt == form.end() || targetIt == form.end()) {
        sendJson(400, jsonError("listen and target are required", "INVALID_REQUEST"));
        return;
    }
    std::string listen;
    std::string target;
    if (!parsePortValue(listenIt->second, listen) || !parsePortValue(targetIt->second, target)) {
        sendJson(400, jsonError("ports must be between 1 and 65535", "INVALID_PORT"));
        return;
    }
    if (!normalizeProto(form.count("proto") ? form.at("proto") : "tcp", proto)) {
        sendJson(400, jsonError("proto must be tcp or udp", "INVALID_PROTO"));
        return;
    }
    if (action != "set" && action != "remove") {
        sendJson(400, jsonError("unsupported port action", "INVALID_ACTION"));
        return;
    }

    ExecResult result;
    if (action == "remove") {
        result = runCapbox({"portmap", "remove", app, "--listen", listen, "--target", target, "--proto", proto, "--user", session.username});
    } else {
        result = runCapbox({"portmap", "set", app, "--listen", listen, "--target", target, "--proto", proto, "--user", session.username});
    }

    if (result.exit_code != 0) {
        const auto output = trim(result.output);
        sendJson(capboxFailureStatus(output), jsonError(output, "PORTMAP_UPDATE_FAILED"));
        return;
    }
    sendJson(200, "{\"ok\":true,\"portmaps\":" + trim(result.output) + "}");
}

void handleDesktopGet(const Session& session) {
    const auto result = runCapbox({"desktop", "get", "--user", session.username});
    if (result.exit_code != 0) {
        sendJson(500, jsonError(trim(result.output)));
        return;
    }
    sendJson(200, trim(result.output));
}

void handleDesktopSet(const Session& session) {
    if (getenvOrEmpty("REQUEST_METHOD") != "POST") {
        sendJson(405, jsonError("method not allowed", "METHOD_NOT_ALLOWED"));
        return;
    }

    const auto form = parseKv(readRequestBody());
    const auto appIt = form.find("app");
    if (appIt == form.end() || appIt->second.empty()) {
        sendJson(400, jsonError("app is required", "INVALID_REQUEST"));
        return;
    }
    if (!validateAppOrSend(appIt->second)) {
        return;
    }

    const auto result = runCapbox({"desktop", "set", appIt->second, "--user", session.username});
    if (result.exit_code != 0) {
        sendJson(400, jsonError(trim(result.output), "DESKTOP_SET_FAILED"));
        return;
    }
    sendJson(200, trim(result.output));
}

void handleUpload(const Session& session, const std::map<std::string, std::string>& query) {
    if (getenvOrEmpty("REQUEST_METHOD") != "POST") {
        sendJson(405, jsonError("method not allowed", "METHOD_NOT_ALLOWED"));
        return;
    }

    std::string filename = "upload.cpk";
    if (const auto it = query.find("filename"); it != query.end() && !it->second.empty()) {
        filename = sanitizeUploadFilename(it->second);
        if (filename != it->second || !isValidUploadFilename(filename)) {
            sendJson(400, jsonError("upload filename may only contain letters, digits, dot, dash and underscore and must end with .cpk, .tgz or .tar.gz", "INVALID_UPLOAD_FILENAME"));
            return;
        }
    } else if (!isValidUploadFilename(filename)) {
        sendJson(400, jsonError("invalid upload filename", "INVALID_UPLOAD_FILENAME"));
        return;
    }

    const auto uploadId = randomHex(8);
    const auto targetPath = uploadPathFor(session.id, uploadId, filename);
    const auto body = readRequestBody();
    if (body.empty()) {
        sendJson(400, jsonError("empty upload body", "EMPTY_UPLOAD"));
        return;
    }

    if (!writeFile(targetPath, body) || !writeFile(uploadMetaPath(session.id, uploadId), targetPath)) {
        sendJson(500, jsonError("failed to save upload"));
        return;
    }

    const auto inspect = runCapbox({"cpk", "validate", targetPath, "--user", session.username});
    if (inspect.exit_code != 0) {
        std::remove(targetPath.c_str());
        std::remove(uploadMetaPath(session.id, uploadId).c_str());
        const auto output = trim(inspect.output);
        sendJson(capboxFailureStatus(output), jsonError(output, "INVALID_CPK"));
        return;
    }

    std::ostringstream out;
    out << "{"
        << "\"ok\":true,"
        << "\"upload\":{"
        << "\"id\":\"" << jsonEscape(uploadId) << "\","
        << "\"filename\":\"" << jsonEscape(filename) << "\""
        << "},"
        << "\"inspection\":" << trim(inspect.output)
        << "}";
    sendJson(200, out.str());
}

void handleInstall(const Session& session) {
    if (getenvOrEmpty("REQUEST_METHOD") != "POST") {
        sendJson(405, jsonError("method not allowed", "METHOD_NOT_ALLOWED"));
        return;
    }

    const auto form = parseKv(readRequestBody());
    const auto uploadIt = form.find("upload_id");
    if (uploadIt == form.end() || !isValidUploadId(uploadIt->second)) {
        sendJson(400, jsonError("upload_id is required", "INVALID_REQUEST"));
        return;
    }

    const auto path = lookupUploadPath(session.id, uploadIt->second);
    if (!path.has_value()) {
        sendJson(404, jsonError("uploaded CPK not found", "UPLOAD_NOT_FOUND"));
        return;
    }

    const auto install = runCapbox({"cpk", "install", *path, "--user", session.username});
    if (install.exit_code != 0) {
        const auto output = trim(install.output);
        sendJson(capboxFailureStatus(output), jsonError(output, "INSTALL_FAILED"));
        return;
    }

    std::remove(path->c_str());
    std::remove(uploadMetaPath(session.id, uploadIt->second).c_str());
    sendJson(200, trim(install.output));
}

}  // namespace

int main() {
    cleanupExpiredSessions();

    const auto path = effectivePathInfo();
    const auto segments = splitPath(path);
    const auto query = parseKv(getenvOrEmpty("QUERY_STRING"));

    if (segments.empty()) {
        handleRoot();
        return 0;
    }

    if (segments[0] == "login") {
        handleLogin();
        return 0;
    }
    if (segments[0] == "logout") {
        handleLogout();
        return 0;
    }
    if (segments[0] == "session") {
        handleSessionInfo();
        return 0;
    }
    if (segments[0] == "hostexec") {
        handleHostexec();
        return 0;
    }

    const auto session = requireSession();
    if (!session.has_value()) {
        return 0;
    }

    if (segments[0] == "me") {
        handleMe(*session);
        return 0;
    }

    if (segments[0] == "system") {
        handleSystem(*session);
        return 0;
    }

    if (segments[0] == "apps") {
        if (segments.size() == 1) {
            handleAppsList(*session);
            return 0;
        }
        if (segments.size() == 2) {
            handleAppInfo(*session, segments[1]);
            return 0;
        }
        if (segments.size() == 3) {
            if (segments[2] == "ports") {
                handleAppPorts(*session, segments[1]);
                return 0;
            }
            if (segments[2] == "reconcile") {
                handleAppReconcile(*session, segments[1]);
                return 0;
            }
            if (segments[2] == "logs") {
                handleAppLogs(*session, segments[1]);
                return 0;
            }
            handleAppAction(*session, segments[1], segments[2]);
            return 0;
        }
    }

    if (segments[0] == "desktop") {
        if (getenvOrEmpty("REQUEST_METHOD") == "GET") {
            handleDesktopGet(*session);
        } else {
            handleDesktopSet(*session);
        }
        return 0;
    }

    if (segments[0] == "upload") {
        handleUpload(*session, query);
        return 0;
    }

    if (segments[0] == "install") {
        handleInstall(*session);
        return 0;
    }

    sendJson(404, jsonError("not found", "NOT_FOUND"));
    return 0;
}
