#pragma once
#include <chrono>
#include <string>
#include <map>
#include <mutex>

class Timer {
public:
    Timer();

    void start();

    
    double stop();

    
    double elapsed() const;

    
    void reportAndCompare(int levelId) const;

    
    static bool loadRecords(const std::string& path = "timerecords.txt");
    static bool saveRecords(const std::string& path = "timerecords.txt");

   
    static double getBestForLevel(int levelId);

private:
    std::chrono::high_resolution_clock::time_point startTime_;
    bool running_;

    static std::map<int, double> bests_;
    static std::mutex mtx_;
    static bool recordsLoaded_;
};