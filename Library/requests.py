from __future__ import annotations

import os
import hashlib
import secrets
import sys
import json as _json
import urllib.request
import urllib.error
from dataclasses import dataclass
from datetime import datetime, timezone
from typing import Dict, List, Optional, Any

from fastapi import FastAPI, Depends, HTTPException, Header, Query, status
from pydantic import BaseModel, EmailStr, Field


# -----------------------------
# (input.py 고정) requests 호환 클라이언트
# -----------------------------
class _SimpleResponse:
    def __init__(self, status_code: int, text: str, headers: Dict[str, str] | None = None):
        self.status_code = status_code
        self.text = text
        self.headers = headers or {}

    def json(self) -> Any:
        return _json.loads(self.text or "null")

def _http_request(method: str, url: str, json: Any = None, headers: Any = None) -> _SimpleResponse:
    hdrs: Dict[str, str] = {}
    if headers is not None:
        try:
            hdrs = dict(headers)
        except Exception:
            hdrs = {}

    data = None
    if json is not None:
        body = _json.dumps(json).encode("utf-8")
        data = body
        hdrs.setdefault("Content-Type", "application/json")

    req = urllib.request.Request(url, data=data, headers=hdrs, method=method.upper())
    try:
        with urllib.request.urlopen(req) as resp:
            text = resp.read().decode("utf-8")
            return _SimpleResponse(status_code=resp.status, text=text, headers=dict(resp.headers))
    except urllib.error.HTTPError as e:
        text = e.read().decode("utf-8") if e.fp else ""
        return _SimpleResponse(status_code=e.code, text=text, headers=dict(e.headers) if e.headers else {})
    except urllib.error.URLError as e:
        raise RuntimeError(f"Failed to connect to {url}: {e}") from e


# input.py에서 쓰는 API
def post(url: str, json: Any = None, headers: Any = None) -> _SimpleResponse:
    return _http_request("POST", url, json=json, headers=headers)


def get(url: str, headers: Any = None) -> _SimpleResponse:
    return _http_request("GET", url, json=None, headers=headers)


# -----------------------------
# 환경설정
# -----------------------------
HOST = os.getenv("APP_HOST", "localhost")
PORT = int(os.getenv("APP_PORT", "8000"))


# -----------------------------
# 데이터 모델 (Pydantic)
# -----------------------------
class SignupRequest(BaseModel):
    username: str = Field(..., min_length=3, max_length=50)
    email: EmailStr
    password: str = Field(..., min_length=6, max_length=128)
    full_name: str = Field(..., min_length=1, max_length=100)


class SignupResponse(BaseModel):
    id: int
    username: str
    email: EmailStr
    full_name: str
    role: str


class LoginRequest(BaseModel):
    username: str
    password: str


class TokenResponse(BaseModel):
    access_token: str
    token_type: str = "bearer"


class BookCreateRequest(BaseModel):
    title: str = Field(..., min_length=1, max_length=200)
    author: str = Field(..., min_length=1, max_length=120)
    isbn: str = Field(..., min_length=10, max_length=20)
    category: str = Field(..., min_length=1, max_length=80)
    total_copies: int = Field(..., ge=1, le=10_000)


class BookOut(BaseModel):
    id: int
    title: str
    author: str
    isbn: str
    category: str
    total_copies: int
    available_copies: int


class BorrowRequest(BaseModel):
    book_id: int = Field(..., ge=1)
    user_id: int = Field(..., ge=1)


class LoanOut(BaseModel):
    id: int
    user_id: int
    book_id: int
    borrowed_at: datetime
    returned_at: Optional[datetime] = None


# -----------------------------
# 내부 저장소 (메모리 DB)
# -----------------------------
@dataclass
class UserRecord:
    id: int
    username: str
    email: str
    full_name: str
    password_hash: str
    role: str  # "admin" | "user"


def _hash_password(pw: str) -> str:
    return hashlib.sha256(pw.encode("utf-8")).hexdigest()


def create_fastapi_app() -> FastAPI:
    app = FastAPI(title="Online Library API", version="0.1.0")

    users_by_id: Dict[int, UserRecord] = {}
    user_id_by_username: Dict[str, int] = {}
    tokens_to_user_id: Dict[str, int] = {}

    books_by_id: Dict[int, BookOut] = {}
    next_book_id = 1

    loans: List[LoanOut] = []
    next_loan_id = 1

    def get_current_user(authorization: str = Header(...)) -> UserRecord:
        if not authorization.lower().startswith("bearer "):
            raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Invalid Authorization header")
        token = authorization.split(" ", 1)[1].strip()
        user_id = tokens_to_user_id.get(token)
        if not user_id or user_id not in users_by_id:
            raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Invalid or expired token")
        return users_by_id[user_id]

    def require_admin(user: UserRecord = Depends(get_current_user)) -> UserRecord:
        if user.role != "admin":
            raise HTTPException(status_code=status.HTTP_403_FORBIDDEN, detail="Admin privileges required")
        return user

    @app.post("/auth/signup", response_model=SignupResponse)
    async def signup(payload: SignupRequest):
        if payload.username in user_id_by_username:
            raise HTTPException(status_code=400, detail="Username already exists")

        new_id = (max(users_by_id.keys()) + 1) if users_by_id else 1
        role = "admin" if new_id == 1 else "user"

        user = UserRecord(
            id=new_id,
            username=payload.username,
            email=str(payload.email),
            full_name=payload.full_name,
            password_hash=_hash_password(payload.password),
            role=role,
        )
        users_by_id[user.id] = user
        user_id_by_username[user.username] = user.id

        return SignupResponse(id=user.id, username=user.username, email=user.email, full_name=user.full_name, role=user.role)

    @app.post("/auth/login", response_model=TokenResponse)
    async def login(payload: LoginRequest):
        uid = user_id_by_username.get(payload.username)
        if not uid:
            raise HTTPException(status_code=401, detail="Invalid credentials")

        user = users_by_id[uid]
        if user.password_hash != _hash_password(payload.password):
            raise HTTPException(status_code=401, detail="Invalid credentials")

        token = secrets.token_urlsafe(32)
        tokens_to_user_id[token] = user.id
        return TokenResponse(access_token=token)

    @app.post("/books", response_model=BookOut)
    async def create_book(payload: BookCreateRequest, admin: UserRecord = Depends(require_admin)):
        nonlocal next_book_id

        for b in books_by_id.values():
            if b.isbn == payload.isbn:
                raise HTTPException(status_code=400, detail="ISBN already exists")

        book = BookOut(
            id=next_book_id,
            title=payload.title,
            author=payload.author,
            isbn=payload.isbn,
            category=payload.category,
            total_copies=payload.total_copies,
            available_copies=payload.total_copies,
        )
        books_by_id[book.id] = book
        next_book_id += 1
        return book

    @app.get("/books", response_model=List[BookOut])
    async def search_books(category: Optional[str] = Query(None), available: Optional[bool] = Query(None)):
        results = list(books_by_id.values())
        if category is not None:
            results = [b for b in results if b.category == category]
        if available is not None:
            results = [b for b in results if (b.available_copies > 0) == available]
        return results

    @app.post("/loans/borrow", response_model=LoanOut)
    async def borrow_book(payload: BorrowRequest, user: UserRecord = Depends(get_current_user)):
        nonlocal next_loan_id

        if payload.user_id != user.id:
            raise HTTPException(status_code=403, detail="Cannot borrow for another user")

        book = books_by_id.get(payload.book_id)
        if not book:
            raise HTTPException(status_code=404, detail="Book not found")
        if book.available_copies <= 0:
            raise HTTPException(status_code=400, detail="No available copies")

        books_by_id[payload.book_id] = book.model_copy(update={"available_copies": book.available_copies - 1})

        loan = LoanOut(
            id=next_loan_id,
            user_id=user.id,
            book_id=payload.book_id,
            borrowed_at=datetime.now(timezone.utc),
            returned_at=None,
        )
        loans.append(loan)
        next_loan_id += 1
        return loan

    @app.get("/users/me/loans", response_model=List[LoanOut])
    async def my_loans(user: UserRecord = Depends(get_current_user)):
        return [l for l in loans if l.user_id == user.id]

    return app


fastapi_app = create_fastapi_app()
__all__ = ["fastapi_app"]

if __name__ == "__main__":
    import uvicorn
    # 직접 실행 시에는 import string 말고 app 객체를 넘기는 게 안전
    uvicorn.run(fastapi_app, host=HOST, port=PORT, reload=True)