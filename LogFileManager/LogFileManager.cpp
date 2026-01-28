#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <thread>

class LogFileManager {
private:
    // 파일명과 파일 스트림을 1:1로 매핑하여 관리
    std::map<std::string, std::unique_ptr<std::fstream>> logFiles;

    // 현재 시간을 [YYYY-MM-DD HH:MM:SS] 형식의 문자열로 생성
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "[%Y-%m-%d %H:%M:%S]");
        return ss.str();
    }

public:
    LogFileManager() = default;

    // 복사 금지 (파일 자원 독점 보장)
    LogFileManager(const LogFileManager&) = delete;
    LogFileManager& operator=(const LogFileManager&) = delete;

    // 이동 허용 (noexcept를 통한 성능 최적화)
    LogFileManager(LogFileManager&&) noexcept = default;
    LogFileManager& operator=(LogFileManager&&) noexcept = default;

    // 1. 로그 파일 오픈
    void openLogFile(const std::string& filename) {
        if (logFiles.find(filename) != logFiles.end()) return;

        // 입출력(in/out) 및 추가(app) 모드 동시 사용
        auto file = std::make_unique<std::fstream>(filename, std::ios::in | std::ios::out | std::ios::app);
        
        if (!file->is_open()) {
            throw std::runtime_error("파일을 열 수 없습니다: " + filename);
        }
        logFiles[filename] = std::move(file);
    }

    // 2. 로그 기록
    void writeLog(const std::string& filename, const std::string& message) {
        auto it = logFiles.find(filename);
        if (it == logFiles.end()) {
            throw std::runtime_error("열려 있지 않은 파일에 기록 시도: " + filename);
        }

        // ios::app 모드이므로 쓰기 포인터는 자동으로 마지막에 위치함
        *(it->second) << getCurrentTimestamp() << " " << message << std::endl;

        if (it->second->fail()) {
            it->second->clear();
            throw std::runtime_error("로그 기록 중 물리적 오류 발생: " + filename);
        }
    }

    // 3. 로그 읽기 (동일한 파일 핸들 사용)
    std::vector<std::string> readLogs(const std::string& filename) {
        auto it = logFiles.find(filename);
        if (it == logFiles.end()) {
            throw std::runtime_error("열려 있지 않은 파일 읽기 시도: " + filename);
        }

        auto& file = *(it->second);
        std::vector<std::string> logs;

        // 포인터 제어: 상태 초기화 후 맨 앞으로 이동
        file.clear(); 
        file.seekg(0, std::ios::beg);

        std::string line;
        while (std::getline(file, line)) {
            logs.push_back(line);
        }

        file.clear(); // 다음 작업을 위해 다시 상태 초기화
        return logs;
    }

    // 4. 로그 파일 닫기
    void closeLogFile(const std::string& filename) {
        auto it = logFiles.find(filename);
        if (it != logFiles.end()) {
            it->second->close();
            logFiles.erase(it); // unique_ptr 소멸 및 맵에서 제거
        }
    }

    ~LogFileManager() = default;
};

int main() {
    try {
        LogFileManager manager;

        // 파일 열기
        manager.openLogFile("error.log");
        manager.openLogFile("debug.log");
        manager.openLogFile("info.log");

        // 로그 기록
        manager.writeLog("error.log", "Database connection failed");
        std::this_thread::sleep_for(std::chrono::seconds(1)); // 1초 대기
        manager.writeLog("debug.log", "User login attempt");
        std::this_thread::sleep_for(std::chrono::seconds(1)); // 1초 대기
        manager.writeLog("info.log", "Server started successfully");

        // 로그 읽기 및 출력
        std::cout << "// error.log 파일 내용" << std::endl;
        std::vector<std::string> errorLogs = manager.readLogs("error.log");
        for (int i = 0; i < (int)errorLogs.size(); ++i) {
            // std::cout << log << std::endl;
            printf("errorLogs[%d]= %s\n", i, errorLogs[i].c_str());
        }
        // 만약 열리지 않은 파일을 쓰려고 하면? (에러 테스트용)
        // manager.writeLog("ghost.log", "Catch me if you can");

    } catch (const std::exception& e) {
        // LogFileManager에서 throw한 에러가 여기로 날아옴
        std::cerr << "에러 발생: " << e.what() << std::endl;
        return 1; // 비정상 종료 코드 반환
    }

    return 0; // 정상 종료
}