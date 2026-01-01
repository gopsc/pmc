#!/usr/bin/env python3
from flask import Flask, request, jsonify
from waitress import serve
import requests

app = Flask(__name__)
allowed_host = [
    'http://192.168.254.101:8001/api',
    'http://192.168.254.100:8006/get_notes',
    'http://127.0.0.1:8006/get_notes'
]

def post_to(url, data):
    response = requests.post(url, json=data)
    return {'code': response.status_code, 'msg': response.text}

@app.route('/api/post_to', methods=['POST'])
def handle_post_to():
    if request.method  == 'POST':
        if request.is_json:
            data = request.get_json()
            url = data['url']
            if url in allowed_host:
                try:
                    d = post_to(url, data['data'])
                    return jsonify(d), 200
                except requests.exceptions.ConnectionError as e:
                    return jsonify({"msg": "CONN ERR"})

            

if __name__ == '__main__':
    serve(app, host='0.0.0.0', port=8005)
