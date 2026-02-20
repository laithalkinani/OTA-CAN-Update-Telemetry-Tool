from flask import Flask, redirect, url_for, render_template
import os

app = Flask(__name__,
            template_folder=os.path.join(os.path.dirname(__file__), 'templates'),
            static_folder=os.path.join(os.path.dirname(__file__), 'static'),
            static_url_path='')

@app.route("/")
def dashboard():
    return render_template("dashboard/index4.html")

@app.route("/login")
def login():
    return render_template("pages/login.html")

@app.route("/pastlog")
def pastlog():
    return render_template("pastlog/pastlog.html")

@app.route("/checkpoint-login")
def checkpoint_login():
    return render_template("checkpoint_login.html")

@app.route("/checkpoint")  
def checkpoint():
    return render_template("checkpoint/checkpoint1.html")

@app.route("/message")
def message():
    return render_template("message/message.html")

if __name__ == "__main__":
    app.run(debug=True)