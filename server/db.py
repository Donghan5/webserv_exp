import sqlite3
from werkzeug.security import generate_password_hash

class Database:
    def __init__(self, db_file="users.db"):
        self.db_file = db_file
        self.init_db()

    def get_connection(self):
        return sqlite3.connect(self.db_file)

    def init_db(self):
        conn = self.get_connection()
        c = conn.cursor()
        
        # Create users table
        c.execute('''
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT UNIQUE NOT NULL,
                password_hash TEXT NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        ''')

        # Create cookies table
        c.execute('''
            CREATE TABLE IF NOT EXISTS cookies (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                user_id INTEGER NOT NULL,
                name TEXT NOT NULL,
                value TEXT NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (user_id) REFERENCES users (id),
                UNIQUE (user_id, name)
            )
        ''')

        # Create sessions table
        c.execute('''
            CREATE TABLE IF NOT EXISTS sessions (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                user_id INTEGER NOT NULL,
                session_token TEXT UNIQUE NOT NULL,
                expires_at TIMESTAMP NOT NULL,
                FOREIGN KEY (user_id) REFERENCES users (id)
            )
        ''')

        # Add default admin user if not exists
        c.execute('SELECT id FROM users WHERE username = ?', ('admin',))
        if not c.fetchone():
            c.execute(
                'INSERT INTO users (username, password_hash) VALUES (?, ?)',
                ('admin', generate_password_hash('password123'))
            )

        conn.commit()
        conn.close()

    def get_user_by_username(self, username):
        conn = self.get_connection()
        c = conn.cursor()
        c.execute('SELECT * FROM users WHERE username = ?', (username,))
        user = c.fetchone()
        conn.close()
        return user if user else None

    def create_user(self, username, password_hash):
        conn = self.get_connection()
        c = conn.cursor()
        try:
            c.execute(
                'INSERT INTO users (username, password_hash) VALUES (?, ?)',
                (username, password_hash)
            )
            conn.commit()
            return True
        except sqlite3.Error as e:
            print(f"Database error: {e}")
            return False
        finally:
            conn.close()

    def add_cookie(self, user_id, name, value):
        conn = self.get_connection()
        c = conn.cursor()
        try:
            c.execute(
                'INSERT OR REPLACE INTO cookies (user_id, name, value) VALUES (?, ?, ?)',
                (user_id, name, value)
            )
            conn.commit()
        except sqlite3.Error as e:
            print(f"Database error: {e}")
            return False
        finally:
            conn.close()
        return True

    def get_user_cookies(self, user_id):
        conn = self.get_connection()
        c = conn.cursor()
        c.execute('SELECT name, value FROM cookies WHERE user_id = ?', (user_id,))
        cookies = {row[0]: row[1] for row in c.fetchall()}
        conn.close()
        return cookies

    def delete_cookie(self, user_id, name):
        conn = self.get_connection()
        c = conn.cursor()
        c.execute('DELETE FROM cookies WHERE user_id = ? AND name = ?', (user_id, name))
        conn.commit()
        conn.close()
        return c.rowcount > 0

    def create_session(self, user_id, session_token, expires_at):
        conn = self.get_connection()
        c = conn.cursor()
        c.execute(
            'INSERT INTO sessions (user_id, session_token, expires_at) VALUES (?, ?, ?)',
            (user_id, session_token, expires_at)
        )
        conn.commit()
        conn.close()

    def get_session(self, session_token):
        conn = self.get_connection()
        c = conn.cursor()
        c.execute('''
            SELECT user_id, expires_at 
            FROM sessions 
            WHERE session_token = ? AND expires_at > datetime('now')
        ''', (session_token,))
        session = c.fetchone()
        conn.close()
        return session

    def delete_session(self, session_token):
        conn = self.get_connection()
        c = conn.cursor()
        c.execute('DELETE FROM sessions WHERE session_token = ?', (session_token,))
        conn.commit()
        conn.close()
