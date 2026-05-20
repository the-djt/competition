#include "timer.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

std::map<int, double> Timer::bests_;
std::mutex Timer::mtx_;
bool Timer::recordsLoaded_ = false;

Timer::Timer()
    : running_(true) {
    startTime_ = std::chrono::high_resolution_clock::now();
}

void Timer::start() {
    startTime_ = std::chrono::high_resolution_clock::now();
    running_ = true;
}

double Timer::stop() {
    if (!running_) {
        return 0.0;
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = endTime - startTime_;
    running_ = false;
    return elapsed.count();
}

double Timer::elapsed() const {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = now - startTime_;
    return elapsed.count();
}

void Timer::reportAndCompare(int levelId) const {
    double current = running_ ? elapsed() : 0.0;
    std::cout << "本次用时: " << std::fixed << std::setprecision(3) << current << " s\n";

    if (levelId <= 0) {
        return;
    }

    std::lock_guard<std::mutex> lk(mtx_);
    auto it = bests_.find(levelId);
    if (it == bests_.end() || current < it->second) {
        if (it == bests_.end()) {
            std::cout << "🎉 首次记录，已设为历史最佳（关卡 " << levelId << "）！\n";
        } else {
            double improvement = it->second - current;
            std::cout << "🎉 新纪录！提升了 " << std::fixed << std::setprecision(3) << improvement << " s\n";
        }
        bests_[levelId] = current;
    } else {
        std::cout << "历史最佳(关 " << levelId << "): " << std::fixed << std::setprecision(3)
                  << it->second << " s\n";
        double gap = current - it->second;
        std::cout << "(距离最佳还有 " << std::fixed << std::setprecision(3) << gap << " s)\n";
    }
}

bool Timer::loadRecords(const std::string& path) {
    std::lock_guard<std::mutex> lk(mtx_);
    bests_.clear();
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        recordsLoaded_ = true; // 允许程序继续，文件不存在视为首次运行
        return false;
    }

    std::string line;
    while (std::getline(ifs, line)) {
        std::istringstream iss(line);
        int levelId;
        double best;
        if (iss >> levelId >> best) {
            bests_[levelId] = best;
        }
    }
    recordsLoaded_ = true;
    return true;
}

bool Timer::saveRecords(const std::string& path) {
    std::lock_guard<std::mutex> lk(mtx_);
    std::ofstream ofs(path, std::ios::trunc);
    if (!ofs.is_open()) {
        return false;
    }
    for (const auto& p : bests_) {
        ofs << p.first << " " << std::fixed << std::setprecision(6) << p.second << "\n";
    }
    return true;
}

double Timer::getBestForLevel(int levelId) {
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = bests_.find(levelId);
    if (it == bests_.end()) return -1.0;
    return it->second;
}