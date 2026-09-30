#pragma once
#include "framework.h"
#include <map>

namespace sibalhook {

// 极简 INI 配置（config.ini），MenuState 持久化用
class Config {
public:
    static Config& Instance();

    void SetPath(const wchar_t* path);
    bool Load();
    bool Save();

    bool        GetBool(const char* sec, const char* key, bool def);
    int         GetInt(const char* sec, const char* key, int def);
    float       GetFloat(const char* sec, const char* key, float def);
    std::string GetString(const char* sec, const char* key, const std::string& def);

    void SetBool(const char* sec, const char* key, bool v);
    void SetInt(const char* sec, const char* key, int v);
    void SetFloat(const char* sec, const char* key, float v);
    void SetString(const char* sec, const char* key, const std::string& v);

private:
    Config() = default;
    std::wstring path_;
    std::map<std::string, std::map<std::string, std::string>> data_;
    std::mutex mtx_;
};

} // namespace sibalhook
