#!/usr/bin/env node
const net = require('net');

/**
 * 发送单条Socket信息
 * @param {string} host - 服务器主机地址
 * @param {number} port = 服务器端口
 * @param {string} message - 要发送的消息
 */

function sendSocketMessage(host, port, message) {
	const client = new net.Socket();
	client.connect(port, host, () => {
		client.write(message);
		client.end();
	});

	client.on('data', (data) => {
	});

	client.on('close', () => {
	});

	client.on('error', (err) => {
		console.error('连接错误', err);
	});
}
// 导出这个函数
module.exports = sendSocketMessage
if (require.main === module) {
	/* 解析参数 */
	sendSocketMessage('qsont.zicp.fun', 10003, 'Hello, Server!');
} else {
	;
}
