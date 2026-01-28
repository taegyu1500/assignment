---

# 개요

본 저장소는 C++을 이용한 핵심 자료구조 및 파일 관리 시스템 구현과 Python FastAPI를 활용한 백엔드 API 데모 프로젝트를 포함하고 있습니다. 상세한 구현 설명 및 시연 결과는 별도의 노션 문서를 통해 제공됩니다.


---

## 1. 프로젝트 구성

### C++ Modules

* **CircularBuffer**: 원형 버퍼(Circular Buffer) 자료구조 직접 구현. Iterator 제공을 통해 `max_element`, `accumulate` 등 STL 알고리즘과의 호환성을 증명하며, Range-based for loop를 지원합니다.
* **LogFileManager**: `std::map`을 활용한 효율적인 로그 파일 관리 시스템입니다. 파일의 Open, Write, Read, Close 등 기본적인 파일 시스템 핸들링 로직을 포함합니다.

### Python Module

* **Library API**: FastAPI 기반의 도서 관리 백엔드 서비스입니다.
* 회원가입, 로그인(Auth), 도서 등록 및 검색, 대출 관리 기능을 포함합니다.
* 외부 라이브러리(`requests`) 의존성 없이 서버와 통신할 수 있는 자체 클라이언트 로직을 포함하고 있습니다.



---

## 2. 개발 및 실행 환경

### C++ (Windows)

* OS: Windows
* Compiler: MinGW-w64 GCC (windows-gcc-x64)
* Standard: C++17 권장
* IDE: VS Code (C/C++ Extension 필수)

### Python

* Version: Python 3.10 이상
* 필수 패키지 설치:
```bash
pip install fastapi uvicorn pydantic

```



---

## 3. 실행 방법

### 3.1 CircularBuffer (C++)

버퍼의 상태 변화를 단계별로 확인할 수 있도록 구성되어 있습니다.

```bash
# 빌드 및 실행
g++ -std=c++17 CircularBuffer.cpp -o CircularBuffer
./CircularBuffer

```

### 3.2 LogFileManager (C++)

기본 동작 확인용 파일과 시연 녹화용(예외 처리 포함) 파일로 구분됩니다.

* **기본 동작 실행:**
```bash
g++ -std=c++17 LogFileManager.cpp -o LogFileManager
./LogFileManager

```


* **시연용 버전 실행 (UTF-8 환경 권장):**
```bash
g++ -std=c++17 LogFileManager_record.cpp -o LogFileManager_record
./LogFileManager_record

```


*참고: Windows 터미널에서 한글 출력이 깨지는 경우 `chcp 65001` 명령어를 선행하십시오.*

### 3.3 Library API (Python FastAPI)

#### 서버 실행

```bash
uvicorn requests:app --reload

```

* 기본 접속 주소: `http://localhost:8000`
* 환경변수 `APP_HOST` 및 `APP_PORT`를 통해 서버 설정 변경이 가능합니다.

#### 클라이언트 시나리오 테스트

서버가 실행 중인 상태에서 별도의 터미널을 통해 시나리오 스크립트를 실행합니다.

```bash
python input.py

```

**테스트 시나리오 구성:**

1. 사용자 회원가입 및 로그인 (Access Token 발급)
2. 도서 등록 (Admin 권한 확인)
3. 전체 도서 목록 검색
4. 도서 대출 신청 및 본인 대출 현황 조회

---

## 4. 기타 사항

* 본 저장소의 코드는 가독성과 유지보수성을 고려하여 작성되었습니다.
* 모든 C++ 코드는 Windows 환경의 MinGW 컴파일러 기준으로 최적화되어 있습니다.
