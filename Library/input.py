import requests
import json
import time

base_url = "http://localhost:8000"

def print_section(title: str):
    print("\n" + "=" * 70)
    print(title)
    print("=" * 70)

def print_response(label: str, resp):
    print(f"\n[{label}] status_code = {resp.status_code}")
    # JSON 우선 출력, 실패하면 text 출력
    try:
        data = resp.json()
        print(json.dumps(data, ensure_ascii=False, indent=2))
    except Exception:
        print(resp.text)

def assert_ok(label: str, resp, ok_codes=(200, 201)):
    if resp.status_code not in ok_codes:
        print_response(label + " (ERROR)", resp)
        raise SystemExit(f"{label} failed with status_code={resp.status_code}")

# 1) 회원가입
print_section("STEP 1) SIGNUP")
signup_data = {
    "username": "john_doe",
    "email": "john@example.com",
    "password": "securepass123",
    "full_name": "John Doe"
}
response = requests.post(f"{base_url}/auth/signup", json=signup_data)
print_response("signup", response)

# 이미 가입된 유저면 400이 날 수 있으니, 영상용으로는 계속 진행 가능하게 처리
if response.status_code not in (200, 201, 400):
    assert_ok("signup", response)

time.sleep(0.6)

# 2) 로그인
print_section("STEP 2) LOGIN")
login_data = {"username": "john_doe", "password": "securepass123"}
auth_response = requests.post(f"{base_url}/auth/login", json=login_data)
print_response("login", auth_response)
assert_ok("login", auth_response)

token = auth_response.json().get("access_token")
if not token:
    raise SystemExit("login response에 access_token이 없습니다.")
headers = {"Authorization": f"Bearer {token}"}
print(f"\n[token] {token[:12]}... (len={len(token)})")

time.sleep(0.6)

# 3) 도서 등록(관리자 권한 필요: 이 서버는 1번 유저를 admin으로 설정)
print_section("STEP 3) CREATE BOOK")
book_data = {
    "title": "Python Programming",
    "author": "John Smith",
    "isbn": "978-0123456789",
    "category": "Programming",
    "total_copies": 5
}
create_book_resp = requests.post(f"{base_url}/books", json=book_data, headers=headers)
print_response("create_book", create_book_resp)

# 이미 ISBN이 존재하면 400일 수 있으니 영상용으로 허용
if create_book_resp.status_code not in (200, 201, 400):
    assert_ok("create_book", create_book_resp)

time.sleep(0.6)

# 4) 도서 검색
print_section("STEP 4) SEARCH BOOKS")
search_response = requests.get(f"{base_url}/books?category=Programming&available=true")
print_response("search_books", search_response)
assert_ok("search_books", search_response)

time.sleep(0.6)

# 5) 대출
print_section("STEP 5) BORROW")
# 서버 로직상: 토큰 유저 id와 user_id가 같아야 함 (첫 가입 유저가 admin=1)
borrow_data = {"book_id": 1, "user_id": 1}
borrow_resp = requests.post(f"{base_url}/loans/borrow", json=borrow_data, headers=headers)
print_response("borrow", borrow_resp)

# 재실행 시 재고 소진/권한 등으로 400/403이 날 수 있음 -> 보여주기용으로 그대로 출력
if borrow_resp.status_code not in (200, 201, 400, 403, 404):
    assert_ok("borrow", borrow_resp)

time.sleep(0.6)

# 6) 내 대출 목록 조회
print_section("STEP 6) MY LOANS")
loans_response = requests.get(f"{base_url}/users/me/loans", headers=headers)
print_response("my_loans", loans_response)
assert_ok("my_loans", loans_response)

print_section("DONE")