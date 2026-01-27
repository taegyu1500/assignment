import requests

base_url = "http://localhost:8000"

signup_data = {
    "username": "john_doe",
    "email": "john@example.com",
    "password": "securepass123",
    "full_name": "John Doe"
}
response = requests.post(f"{base_url}/auth/signup", json=signup_data)

login_data = {"username": "john_doe", "password": "securepass123"}
auth_response = requests.post(f"{base_url}/auth/login", json=login_data)
token = auth_response.json()["access_token"]
headers = {"Authorization": f"Bearer {token}"}

book_data = {
    "title": "Python Programming",
    "author": "John Smith",
    "isbn": "978-0123456789",
    "category": "Programming",
    "total_copies": 5
}
requests.post(f"{base_url}/books", json=book_data, headers=headers)

search_response = requests.get(f"{base_url}/books?category=Programming&available=true")
borrow_data = {"book_id": 1, "user_id": 1}
requests.post(f"{base_url}/loans/borrow", json=borrow_data, headers=headers)

loans_response = requests.get(f"{base_url}/users/me/loans", headers=headers)