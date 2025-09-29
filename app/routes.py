from flask import Blueprint, render_template, request, redirect, url_for, session
import json, os
from werkzeug.security import generate_password_hash, check_password_hash

main = Blueprint("main", __name__)

USERS_FILE = "users.json"

def load_users():
    if not os.path.exists(USERS_FILE):
        return {}
    with open(USERS_FILE, "r") as f:
        return json.load(f)

def save_users(users):
    with open(USERS_FILE, "w") as f:
        json.dump(users, f, indent=4)

@main.route("/")
def home():
    if "user" in session:
        return redirect(url_for("main.dashboard"))
    return redirect(url_for("main.login"))

# ---------- Registro ----------
@main.route("/register", methods=["GET", "POST"])
def register():
    if request.method == "POST":
        username = request.form["username"]
        email = request.form["email"]
        password = request.form["password"]

        users = load_users()
        if username in users:
            return render_template("register.html", error="Usuario ya existe")

        users[username] = {
            "email": email,
            "password": generate_password_hash(password)
        }
        save_users(users)
        return redirect(url_for("main.login"))
    return render_template("register.html")

# ---------- Login ----------
@main.route("/login", methods=["GET", "POST"])
def login():
    if request.method == "POST":
        username = request.form["username"]
        password = request.form["password"]

        users = load_users()
        if username in users and check_password_hash(users[username]["password"], password):
            session["user"] = username
            return redirect(url_for("main.dashboard"))
        else:
            return render_template("login.html", error="Credenciales inválidas")
    return render_template("login.html")

# ---------- Dashboard ----------
@main.route("/dashboard")
def dashboard():
    if "user" not in session:
        return redirect(url_for("main.login"))
    return render_template("dashboard.html", user=session["user"])

# ---------- Logout ----------
@main.route("/logout")
def logout():
    session.pop("user", None)
    return redirect(url_for("main.login"))
