#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>

namespace bfb {

struct PlayerProfile {
    float volume = 0.65F;
    bool muted = false;
    bool tutorialSeen = false;
    std::map<int, double> bestTimes;
};

class ProfileStore {
public:
    explicit ProfileStore(std::filesystem::path path = defaultPath());

    PlayerProfile load(std::string* warning = nullptr) const;
    bool save(const PlayerProfile& profile, std::string* warning = nullptr) const;
    const std::filesystem::path& path() const { return path_; }

    static std::filesystem::path defaultPath();

private:
    std::filesystem::path path_;
};

}  // namespace bfb
