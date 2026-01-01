#!/usr/bin/env node
//import {httpRequest}  from "./http_request.js";
const { httpRequest } = require("./http_request.js");
//import { Command } from "commander"
const { Command } = require("commander");


async function getFromCops(addr, port, cmd) {
    return httpRequest({
                    hostname: addr,
                    port: Number(port),
                    path: '/api',
                    method: 'POST',
                    data: {"cmd": cmd}
                });
}
function main() {
    const program = new Command();
    program
        .name("call_cops")
        .arguments('<addr> <port> <cmd>')
        .description('调用协处理器必选参数：addr, port, cmd')
        .action((addr, port, cmd) => {
            const getResponse = getFromCops(addr, port, cmd)
            getResponse.then(result => {
                console.log('响应：', getResponse)
            })
        })
        program.parse(process.argv);
}
main()
