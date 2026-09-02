#include "core/profile_store.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>

namespace bfb {
namespace {

std::optional<std::string> capture(const std::string& text, const std::regex& pattern) {
    std::smatch match;
    if (std::regex_search(text, match, pattern) && match.size() > 1) return match[1].str();
    return std::nullopt;
}

}  // namespace

ProfileStore::ProfileStore(std::filesystem::path path) : path_(std::move(path)) {}

std::filesystem::path ProfileStore::defaultPath() {
    if (const char* local = std::getenv("LOCALAPPDATA")) {
        return std::filesystem::path(local) / "BattlefrontBreakout" / "profile.json";
    }
    return std::filesystem::current_path() / "save" / "profile.json";
}

PlayerProfile ProfileStore::load(std::string* warning) const {
    PlayerProfile profile;
    std::ifstream stream(path_, std::ios::binary);
    if (!stream) return profile;
    const std::string text((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    try {
        if (const auto value = capture(text, std::regex(R"("volume"\s*:\s*([0-9.]+))"))) {
            profile.volume = std::clamp(std::stof(*value), 0.0F, 1.0F);
        }
        if (const auto value = capture(text, std::regex(R"("muted"\s*:\s*(true|false))"))) {
            profile.muted = *value == "true";
        }
        if (const auto value = capture(text, std::regex(R"("tutorialSeen"\s*:\s*(true|false))"))) {
            profile.tutorialSeen = *value == "true";
        }
        const std::regex timePattern(R"REGEX("([0-9]+)"\s*:\s*([0-9.]+))REGEX");
        const auto begin = std::sregex_iterator(text.begin(), text.end(), timePattern);
        const auto end = std::sregex_iterator();
        for (auto it = begin; it != end; ++it) {
            const int level = std::stoi((*it)[1].str());
            const double seconds = std::stod((*it)[2].str());
            if (level >= 1 && level <= 10 && seconds >= 0.0) profile.bestTimes[level] = seconds;
        }
    } catch (const std::exception&) {
        if (warning) *warning = "存档格式异常，已使用默认设置";
        return PlayerProfile{};
    }
    return profile;
}

bool ProfileStore::save(const PlayerProfile& profile, std::string* warning) const {
    std::error_code error;
    std::filesystem::create_directories(path_.parent_path(), error);
    if (error) {
        if (warning) *warning = "无法创建存档目录";
        return false;
    }
    const auto temporary = path_.string() + ".tmp";
    std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
    if (!stream) {
        if (warning) *warning = "无法写入存档";
        return false;
    }
    stream << "{\n"
           << "  \"volume\": " << std::fixed << std::setprecision(2)
           << std::clamp(profile.volume, 0.0F, 1.0F) << ",\n"
           << "  \"muted\": " << (profile.muted ? "true" : "false") << ",\n"
           << "  \"tutorialSeen\": " << (profile.tutorialSeen ? "true" : "false") << ",\n"
           << "  \"bestTimes\": {";
    bool first = true;
    for (const auto& [level, seconds] : profile.bestTimes) {
        if (!first) stream << ',';
        stream << "\n    \"" << level << "\": " << std::setprecision(3) << seconds;
        first = false;
    }
    if (!profile.bestTimes.empty()) stream << '\n';
    stream << "  }\n}\n";
    stream.close();
    if (!stream) {
        if (warning) *warning = "存档写入未完成";
        return false;
    }
    std::filesystem::rename(temporary, path_, error);
    if (error) {
        std::filesystem::remove(path_, error);
        error.clear();
        std::filesystem::rename(temporary, path_, error);
    }
    if (error) {
        if (warning) *warning = "无法替换旧存档";
        return false;
    }
    return true;
}

}  // namespace bfb
