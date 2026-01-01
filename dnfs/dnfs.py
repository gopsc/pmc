#!/usr/bin/env python3





'''

Dendrachy Network File-System (dnfs)

树状网络文件系统



可能加入数据库加密功能。

'''

from flask import Flask, request, jsonify
from flask_cors import CORS

from flask_sqlalchemy import SQLAlchemy

from waitress import serve

import os

basedir = os.path.abspath(os.path.dirname(__file__))

app = Flask(__name__)
CORS(app)

app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///' + os.path.join(basedir, 'notes.db')

app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False

db = SQLAlchemy(app)

class Note(db.Model):

    id = db.Column(db.Integer, primary_key=True)

    pid = db.Column(db.Integer, nullable=False)

    title = db.Column(db.String(128), nullable=False)

    content = db.Column(db.Text, nullable=False)



    def __repr__(self):

        return f"<Node {self.title}>"

with app.app_context():

    db.create_all()


@app.route('/', methods=['GET'])
def index():
    return "dnfs运行中",  200

# eg: {
#    "pid": "0",
#    "title": "test",
#    "content": "hello"
#} 
@app.route('/add_note', methods=['POST'])

def add_note():

    data = request.get_json()

    pid = data.get('pid')

    title = data.get('title')

    content = data.get('content')

    if pid is None or not title or not content:

        return jsonify({"error": "Pid, title and content are required"}), 400

    new_note = Note(pid=int(pid), title=title, content=content)

    with app.app_context():

        db.session.add(new_note)

        db.session.commit()

    return jsonify({"message": "Note added successfully"}), 200


# {“pid”: "0"} 
@app.route('/get_notes', methods=["POST"])

def get_notes():

    data = request.get_json()

    pid = data.get('pid')

    if not pid:

        pid = 0

    with app.app_context():

        notes = Note.query.filter(Note.pid == int(pid))

        result = [{'id': note.id, 'pid': note.pid, 'title': note.title, 'content':note.content} for note in notes]

    return jsonify(result), 200


# eg: {"id": "1"}
@app.route('/delete_note', methods=['POST'])

def delete_note():

    data = request.get_json()

    note_id = data.get('id')

    if not note_id:

        return jsonify({"error": "Note ID is required"}), 400

    with app.app_context():

        note = Note.query.get(note_id)

        if note:

            db.session.delete(note)

            db.session.commit()

            return jsonify({'message': "Note deleted successfully"}), 200

        else:

            return jsonify({"error": "Note not found"}), 404


# eg: {
#    "id": "1",
#    "pid": "0",
#    "title": "test",
#    "content": "hello"
#}
@app.route('/update_note', methods=['POST'])

def update_note():

    data = request.get_json()

    id = data.get('id')

    pid  = data.get('pid')

    title = data.get('title')

    content = data.get('content')

    if id is None or pid is None or not title or not content:

        return jsonify({"error": "Invalid param"}), 400

    with app.app_context():

        note = Note.query.get(id)

        if note:

            note.pid = pid

            note.title = title

            note.content = content

            db.session.commit()

            return jsonify({'message': "Your change is up-to-date."}), 200

        else:

            return jsonify({'error': 'Note not found'}), 400




if __name__ == "__main__":
    #app.run(host="0.0.0.0", port=8006)
    serve(app, host='0.0.0.0', port=8006)
