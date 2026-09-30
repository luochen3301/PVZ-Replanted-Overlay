#include "config/config.h"
#include "paths.h"

namespace sibalhook {

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}
static std::string lower(std::string s) {
    for (auto& c : s) c = (char)tolower((unsigned char)c);
    return s;
}

Config& Config::Instance() {
    static Config inst;
    return inst;
}

void Config::SetPath(const wchar_t* path) {
    std::lock_guard<std::mutex> lk(mtx_);
    path_ = path ? path : L"";
}

bool Config::Load() {
    std::lock_guard<std::mutex> lk(mtx_);
    data_.clear();
    if (path_.empty()) path_ = ROH_CONFIG_PATH;
    FILE* f = _wfopen(path_.c_str(), L"rb");
    if (!f) return false;
    std::string cur;
    char buf[2048];
    while (fgets(buf, sizeof(buf), f)) {
        std::string line = trim(buf);
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        if (line.front() == '[' && line.back() == ']') {
            cur = lower(line.substr(1, line.size() - 2));
            continue;
        }
        auto eq = line.find('=');
        if (eq == std::string::npos || cur.empty()) continue;
        data_[cur][lower(trim(line.substr(0, eq)))] = trim(line.substr(eq + 1));
    }
    fclose(f);
    return true;
}

bool Config::Save() {
    std::lock_guard<std::mutex> lk(mtx_);
    if (path_.empty()) path_ = ROH_CONFIG_PATH;
    FILE* f = _wfopen(path_.c_str(), L"wb");
    if (!f) return false;
    for (auto& sec : data_) {
        fprintf(f, "[%s]\n", sec.first.c_str());
        for (auto& kv : sec.second) fprintf(f, "%s=%s\n", kv.first.c_str(), kv.second.c_str());
        fprintf(f, "\n");
    }
    fclose(f);
    return true;
}

static const std::string kEmpty;
std::string* find_val(std::map<std::string, std::map<std::string, std::string>>& d,
                      const char* sec, const char* key) {
    auto it = d.find(lower(sec));
    if (it == d.end()) return nullptr;
    auto it2 = it->second.find(lower(key));
    if (it2 == it->second.end()) return nullptr;
    return &it2->second;
}

bool Config::GetBool(const char* sec, const char* key, bool def) {
    std::lock_guard<std::mutex> lk(mtx_);
    auto* v = find_val(data_, sec, key);
    return v ? (lower(*v) == "1" || lower(*v) == "true" || lower(*v) == "yes") : def;
}
int Config::GetInt(const char* sec, const char* key, int def) {
    std::lock_guard<std::mutex> lk(mtx_);
    auto* v = find_val(data_, sec, key);
    return v ? atoi(v->c_str()) : def;
}
float Config::GetFloat(const char* sec, const char* key, float def) {
    std::lock_guard<std::mutex> lk(mtx_);
    auto* v = find_val(data_, sec, key);
    return v ? (float)atof(v->c_str()) : def;
}
std::string Config::GetString(const char* sec, const char* key, const std::string& def) {
    std::lock_guard<std::mutex> lk(mtx_);
    auto* v = find_val(data_, sec, key);
    return v ? *v : def;
}
void Config::SetBool(const char* sec, const char* key, bool v) {
    std::lock_guard<std::mutex> lk(mtx_);
    data_[lower(sec)][lower(key)] = v ? "1" : "0";
}
void Config::SetInt(const char* sec, const char* key, int v) {
    std::lock_guard<std::mutex> lk(mtx_);
    data_[lower(sec)][lower(key)] = std::to_string(v);
}
void Config::SetFloat(const char* sec, const char* key, float v) {
    std::lock_guard<std::mutex> lk(mtx_);
    data_[lower(sec)][lower(key)] = std::to_string(v);
}
void Config::SetString(const char* sec, const char* key, const std::string& v) {
    std::lock_guard<std::mutex> lk(mtx_);
    data_[lower(sec)][lower(key)] = v;
}

} // namespace sibalhook
