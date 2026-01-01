#!/usr/bin/env node
//const { rejects, partialDeepStrictEqual } = require('assert')
//const { error } = require('console')
const http = require('http')
const https = require('https')
//const { resolve } = require('path')
//import http from 'http'
//import https from "https"
/**
 * 发送HTTP请求（封装成Promise）
 * @param {Object} options - 请求配置
 * @param {string} [options.method='GET'] - 请求方法（GET/POST等）
 * @param {string} options.hostname - 主机名（如'127.0.0.1'）
 * @param {number} [options.port=80] - 端口号
 * @param {string} options.path - 请求路径（如'/api?key=value'）
 * @param {Object} [options.headers={}] - 请求头
 * @param {Object} [options.data=null] - POST请求的数据（自动转为JSON）
 * @param {boolean} [options.https=false] - 是否使用HTTPS
 * @returns {Promise<Object>} 返回响应数据（自动解析JSON）
 */
function httpRequest(options) {
    return new Promise((resolve, reject) => {
        // 默认值处理
        const {
            method = 'GET',
            hostname,
            port = options.https ? 443 : 80,
            path,
            headers = {},
            data = null,
            https = false,
        } = options;

        // 选择HTTP或HTTPS模块
        const protocol = https ? https : http

        // 准备请求配置
        const requestOptions = {
            method,
            hostname,
            port,
            path,
            headers
        };

        //如果是POST请求，添加Content-Type和Content-Length
        if (method.toUpperCase() === "POST" && data) {
            const postData = JSON.stringify(data);
            requestOptions.headers = {
                ...headers,
                'Content-Type': 'application/json',
                'Content-Length': postData.length
            };
        }

        //创建请求
        const req = protocol.request(requestOptions, (res) => {
            let responseData = '';

            //接收数据
            res.on('data', (chunk) => {
                responseData += chunk;
            });

            // 请求结束
            res.on('end', () => {
                let parsedData = ""
                try {
                    // 尝试解析JSON（如果是JSON相应）
                    parsedData = responseData ? JSON.parse(responseData) : {};
                } catch (error) {
                    //reject(new Error(`响应数据解析失败：${error.message}`))
                    parsedData = responseData;
                }
                resolve({
                    statesCode: res.statusCode,
                    headers: res.headers,
                    data: parsedData
                });
            });
        });

        //错误处理
        req.on('error', (error) => {
            reject(new Error(`请求失败：${error.message}`));
        });

        // 如果是POST请求，发送数据
        if (method.toUpperCase() === "POST" && data) {
            req.write(JSON.stringify(data));
        }

        //请求结束
        req.end();
    });
}

// 示例用法
async function demo() {
    try {
        const getResponse = await httpRequest({
            hostname: '192.168.254.101',
            port: 8001,
            path: '/api',
            method: 'POST',
            data: {"cmd": "MPU6050"}
        });
        console.log('响应：', getResponse);

        // POST请求示例
        const postResponse = await httpRequest({
            hostname: '127.0.0.1',
            port: 8012,
            path: '/api?call=shell&path=ls',
            method: "POST",
            data: ['-a']
        });
        console.log("POST响应：", postResponse);
    }    catch(error) {
        console.error('请求出错：',error);
    }
}
