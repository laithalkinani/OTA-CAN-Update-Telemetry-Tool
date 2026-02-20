from flask import Flask, redirect, url_for, render_template
import os

app = Flask(__name__,
            template_folder=os.path.join(os.path.dirname(__file__), 'templates'),
            static_folder=os.path.join(os.path.dirname(__file__), 'static'),
            static_url_path='')

@app.route("/")
def dashboard():
    return render_template("dashboard/index2.html")

@app.route("/login")
def login():
    return render_template("pages/login-v1.html")

@app.route("/register")
def register():
    return render_template("pages/register-v1.html")

@app.route("/sample")
def sample():
    return render_template("other/sample-page.html")

@app.route("/typography")
def typography():
    return render_template("elements/bc_typography.html")

@app.route("/color")
def color():
    return render_template("elements/bc_color.html")

@app.route("/icons")
def icons():
    return render_template("elements/icon-material.html")

@app.route("/pastlog")
def pastlog():
    return render_template("pastlog/pastlog.html")

@app.route("/checkpoint")  
def checkpoint():
    return render_template("checkpoint/checkpoint.html")

@app.route("/message")
def message():
    return render_template("message/message.html")

if __name__ == "__main__":
    app.run(debug=True)