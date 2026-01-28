#include <iostream>
#include <vector>
#include <algorithm> // max_element 사용
#include <numeric>   // accumulate 사용
#include <iomanip>   // 출력 포맷 설정
#include <iterator>

// 추가: 라벨/슬립용
#include <string>
#include <chrono>
#include <thread>

template <typename T>
class CircularBuffer {
private:
    T* buffer;
    int head = 0;      // 가장 오래된 데이터의 인덱스
    int tail = -1;     // 가장 최근에 추가된 데이터의 인덱스
    size_t m_size = 0;
    size_t m_capacity;

    void inc_head() { head = (head + 1) % (int)m_capacity; }
    void inc_tail() { tail = (tail + 1) % (int)m_capacity; }

public:
    CircularBuffer(size_t capacity) : m_capacity(capacity) {
        buffer = new T[m_capacity];
    }

    ~CircularBuffer() { delete[] buffer; }

    size_t size() const { return m_size; }
    size_t capacity() const { return m_capacity; }
    bool empty() const { return m_size == 0; }

    void pop_front() {
        if (empty()) return;
        inc_head();
        m_size--;
    }

    void push_back(const T& item) {
        if (m_size == m_capacity) {
            pop_front();
        }
        inc_tail();
        buffer[tail] = item;
        m_size++;
    }

    T& front() { return buffer[head]; }
    const T& front() const { return buffer[head]; }

    T& back() { return buffer[tail]; }
    const T& back() const { return buffer[tail]; }

    class Iterator {
    private:
        CircularBuffer* ptr;
        size_t offset;
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        Iterator(CircularBuffer* p, size_t o) : ptr(p), offset(o) {}

        reference operator*() const {
            return ptr->buffer[(ptr->head + offset) % ptr->m_capacity];
        }

        Iterator& operator++() { offset++; return *this; }

        bool operator==(const Iterator& other) const {
            return ptr == other.ptr && offset == other.offset;
        }
        bool operator!=(const Iterator& other) const { return !(*this == other); }
    };

    Iterator begin() { return Iterator(this, 0); }
    Iterator end() { return Iterator(this, m_size); }

    // 추가: 영상용 상태 출력(논리 순서 + raw 배열 + head/tail 표시)
    void debug_print(const std::string& label = "") const {
        if (!label.empty()) std::cout << "\n==== " << label << " ====\n";
        std::cout << "size=" << m_size << "/" << m_capacity
                  << ", head=" << head << ", tail=" << tail << "\n";

        std::cout << "logical order: ";
        for (size_t i = 0; i < m_size; ++i) {
            std::cout << buffer[(head + (int)i) % (int)m_capacity] << " ";
        }
        std::cout << "\nraw slots:     ";
        for (size_t i = 0; i < m_capacity; ++i) {
            if ((int)i == head) std::cout << "H";
            if ((int)i == tail) std::cout << "T";
            std::cout << "[" << i << "]=" << buffer[i] << " ";
        }
        std::cout << "\n";
    }
};

// --- 테스트 메인 함수 (과제 이미지 시나리오) ---
int main() {
    CircularBuffer<double> tempBuffer(5);

    // 추가: 단계 출력 + 딜레이(영상에서 변화가 보이게)
    auto step = [&](const char* label) {
        tempBuffer.debug_print(label);
        std::this_thread::sleep_for(std::chrono::milliseconds(700));
    };

    step("start (empty)");

    tempBuffer.push_back(23.5); step("push_back(23.5)");
    tempBuffer.push_back(24.1); step("push_back(24.1)");
    tempBuffer.push_back(23.8); step("push_back(23.8)");
    tempBuffer.push_back(25.2); step("push_back(25.2)");
    tempBuffer.push_back(24.7); step("push_back(24.7)");
    tempBuffer.push_back(26.1); step("push_back(26.1) -> overwrite oldest");

    // 2. 결과 출력
    std::cout << "tempBuffer.size() = " << tempBuffer.size() << std::endl;
    std::cout << "tempBuffer.capacity() = " << tempBuffer.capacity() << std::endl;
    std::cout << "tempBuffer.empty() = " << (tempBuffer.empty() ? "true" : "false") << std::endl;

    // 3. STL 알고리즘 활용
    double maxTemp = *std::max_element(tempBuffer.begin(), tempBuffer.end());
    double sum = std::accumulate(tempBuffer.begin(), tempBuffer.end(), 0.0);
    double avgTemp = sum / tempBuffer.size();

    std::cout << "maxTemp = " << maxTemp << std::endl;
    std::cout << "avgTemp = " << std::fixed << std::setprecision(2) << avgTemp << std::endl;

    // 4. front/back 확인
    std::cout << "tempBuffer.front() = " << tempBuffer.front() << " // 가장 오래된 데이터" << std::endl;
    std::cout << "tempBuffer.back() = " << tempBuffer.back() << " // 가장 최근 데이터" << std::endl;

    // 5. 범위 기반 for문 확인
    std::cout << "전체 데이터 순회: ";
    for (const auto& temp : tempBuffer) {
        std::cout << temp << " ";
    }
    std::cout << std::endl;

    return 0;
}