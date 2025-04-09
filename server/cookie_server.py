from flask import Flask, request, jsonify, make_response, session
from flask_cors import CORS
import logging
from werkzeug.security import check_password_hash, generate_password_hash
from datetime import datetime, timedelta
import secrets
from db import Database
import functools

app = Flask(__name__)
app.secret_key = secrets.token_hex(16)
CORS(app, supports_credentials=True)

# Initialize database
db = Database()

# Enable debug logging
logging.basicConfig(level=logging.DEBUG)

def require_auth(f):
    @functools.wraps(f)
    def decorated(*args, **kwargs):
        session_token = request.cookies.get('session')
        if not session_token:
            return jsonify({'error': 'No session'}), 401

        session = db.get_session(session_token)
        if not session:
            return jsonify({'error': 'Invalid or expired session'}), 401

        request.user_id = session[0]  # Set user_id for the route handler
        return f(*args, **kwargs)
    return decorated

@app.route('/api/login', methods=['POST'])
def login():
    app.logger.debug("Login attempt")
    username = request.form.get('username')
    password = request.form.get('password')

    user = db.get_user_by_username(username)
    if user and check_password_hash(user[2], password):
        # Generate session token and expiration
        session_token = secrets.token_hex(32)
        expires = datetime.now() + timedelta(hours=24)

        # Store session
        db.create_session(user[0], session_token, expires)

        response = make_response(jsonify({
            'status': 'success',
            'message': 'Login successful'
        }))

        # Set secure session cookie
        response.set_cookie(
            'session',
            session_token,
            httponly=True,
            secure=True,  # Enable in production
            samesite='Strict',
            expires=expires
        )
        return response

    return jsonify({
        'status': 'error',
        'message': 'Invalid credentials'
    }), 401

@app.route('/api/register', methods=['POST'])
def register():
    app.logger.debug("Registration attempt")
    username = request.form.get('username')
    password = request.form.get('password')

    if not username or not password:
        return jsonify({
            'status': 'error',
            'message': 'Username and password are required'
        }), 400

    # Check if username already exists
    existing_user = db.get_user_by_username(username)
    if existing_user:
        return jsonify({
            'status': 'error',
            'message': 'Username already exists'
        }), 409

    # Create new user
    if db.create_user(username, generate_password_hash(password)):
        return jsonify({
            'status': 'success',
            'message': 'Registration successful'
        })

    return jsonify({
        'status': 'error',
        'message': 'Registration failed'
    }), 500

@app.route('/api/logout', methods=['POST'])
def logout():
    session_token = request.cookies.get('session')
    if session_token:
        db.delete_session(session_token)

    response = make_response(jsonify({'message': 'Logged out'}))
    response.delete_cookie('session')
    return response

@app.route('/api/cookies', methods=['GET', 'POST', 'DELETE'])
@require_auth
def handle_cookies():
    if request.method == 'GET':
        cookies = db.get_user_cookies(request.user_id)
        return jsonify(cookies)

    elif request.method == 'POST':
        name = request.form.get('cookieName')
        value = request.form.get('cookieValue')

        if not name or not value:
            return jsonify({'error': 'Missing cookie name or value'}), 400

        # Store in database
        if db.add_cookie(request.user_id, name, value):
            # Create response and set browser cookie
            response = make_response(jsonify({'message': 'Cookie set successfully'}))
            response.set_cookie(
                name,
                value,
                httponly=False,  # Allow JavaScript access
                secure=True,    # Require HTTPS in production
                samesite='Lax'  # Allow cross-site requests
            )
            return response
        return jsonify({'error': 'Failed to set cookie'}), 500

    elif request.method == 'DELETE':
        name = request.args.get('name')
        if not name:
            return jsonify({'error': 'Missing cookie name'}), 400

        # Delete from database
        success = db.delete_cookie(request.user_id, name)
        if success:
            # Create response and delete browser cookie
            response = make_response(jsonify({'message': 'Cookie deleted successfully'}))
            response.delete_cookie(name)
            return response
        return jsonify({'error': 'Cookie not found'}), 404

if __name__ == '__main__':
    app.run(host='localhost', port=8080)
