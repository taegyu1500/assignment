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

// 추가: Windows 콘솔 UTF-8(한글 출력 깨짐 방지용, 필요 없으면 지워도 됨)
#ifdef _WIN32
#include <windows.h>
#endif

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

// ====== 영상용 유틸(출력/딜레이/초기화) ======
static void InitConsoleUtf8() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

static void SleepMs(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

static void PrintSection(const std::string& title) {
    std::cout << "\n============================================================\n";
    std::cout << title << "\n";
    std::cout << "============================================================\n";
}

static void TruncateFile(const std::string& filename) {
    std::ofstream ofs(filename, std::ios::trunc); // 내용 비우기
}

static void PrintLogs(const std::string& filename, const std::vector<std::string>& logs) {
    std::cout << "\n[" << filename << "] lines=" << logs.size() << "\n";
    for (size_t i = 0; i < logs.size(); ++i) {
        std::cout << "  " << std::setw(2) << i << ": " << logs[i] << "\n";
    }
}

int main() {
    InitConsoleUtf8();

    try {
        PrintSection("DEMO: LogFileManager (open/write/read/exception/close)");
        std::cout << "작업 디렉터리에 error.log / debug.log / info.log 파일이 생성됩니다.\n";
        SleepMs(800);

        // 0) 항상 같은 출력이 나오도록 기존 로그 비우기(영상용)
        PrintSection("STEP 0) truncate existing log files (clean start)");
        TruncateFile("error.log");
        TruncateFile("debug.log");
        TruncateFile("info.log");
        std::cout << "기존 로그 파일 내용을 비웠습니다.\n";
        SleepMs(800);

        LogFileManager manager;

        // 1) 파일 열기
        PrintSection("STEP 1) openLogFile()");
        manager.openLogFile("error.log");
        manager.openLogFile("debug.log");
        manager.openLogFile("info.log");
        std::cout << "3개 파일 open 완료\n";
        SleepMs(800);

        // 2) 로그 기록(딜레이로 타임스탬프 변화가 보이게)
        PrintSection("STEP 2) writeLog() with delay");
        manager.writeLog("info.log",  "Server starting...");
        SleepMs(500);
        manager.writeLog("debug.log", "Config loaded: port=8080");
        SleepMs(500);
        manager.writeLog("error.log", "Database connection failed (retry=1)");
        SleepMs(500);
        manager.writeLog("debug.log", "Retrying database connection...");
        SleepMs(500);
        manager.writeLog("info.log",  "Server started successfully");
        std::cout << "로그 5줄 기록 완료\n";
        SleepMs(900);

        // 3) 로그 읽기 + 출력(파일별로)
        PrintSection("STEP 3) readLogs() and print");
        auto errorLogs = manager.readLogs("error.log");
        auto debugLogs = manager.readLogs("debug.log");
        auto infoLogs  = manager.readLogs("info.log");

        PrintLogs("error.log", errorLogs);
        PrintLogs("debug.log", debugLogs);
        PrintLogs("info.log",  infoLogs);

        // 요약 출력(영상에서 한눈에 보이게)
        std::cout << "\n[Summary]\n";
        std::cout << "  error.log lines = " << errorLogs.size() << "\n";
        std::cout << "  debug.log lines = " << debugLogs.size() << "\n";
        std::cout << "  info.log  lines = " << infoLogs.size() << "\n";
        SleepMs(900);

        // 4) 예외 상황 시연(프로그램이 죽지 않고 처리되는지)
        PrintSection("STEP 4) exception demo (write to unopened file)");
        try {
            manager.writeLog("ghost.log", "This should throw");
            std::cout << "ERROR: should not reach here\n";
        } catch (const std::exception& e) {
            std::cout << "예상대로 예외를 잡았습니다: " << e.what() << "\n";
        }
        SleepMs(900);

        // 5) 닫기
        PrintSection("STEP 5) closeLogFile()");
        manager.closeLogFile("error.log");
        manager.closeLogFile("debug.log");
        manager.closeLogFile("info.log");
        std::cout << "3개 파일 닫기 완료\n";
        SleepMs(800);

    } catch (const std::exception& e) {
        // LogFileManager에서 throw한 에러가 여기로 날아옴
        std::cerr << "에러 발생: " << e.what() << std::endl;
        return 1; // 비정상 종료 코드 반환
    }

    return 0; // 정상 종료
}