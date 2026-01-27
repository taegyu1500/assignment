from flask import Flask, jsonify, request
from fastapi import FastAPI

def create_flask_app():
    app = Flask(__name__)

    @app.route('/flask/hello', methods=['GET'])
    def hello_flask():
        return jsonify(message="Hello from Flask!")

    @app.route('/flask/echo', methods=['POST'])
    def echo_flask():
        data = request.json
        return jsonify(data)

    return app

def create_fastapi_app():
    app = FastAPI()

    @app.get("/fastapi/hello")
    async def hello_fastapi():
        return {"message": "Hello from FastAPI!"}

    @app.post("/fastapi/echo")
    async def echo_fastapi(data: dict):
        return data

    return app

flask_app = create_flask_app()
fastapi_app = create_fastapi_app()
__all__ = ['flask_app', 'fastapi_app']

if __name__ == "__main__":
    flask_app.run(debug=True)
    import uvicorn
    uvicorn.run(fastapi_app, host="127.0.0.1", port=8000)
    
