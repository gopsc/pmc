#!/usr/bin/env lua

local http = require("socket.http")
local https = require("ssl.https")
local ltn12 = require("ltn12")

local M = {}
--- HTTP请求函数
-- @param url     请求地址
-- @param method  请求方法（GET/POST/PUT/DELETE）
-- @param headers 请求头（可选）
-- @param body    请求体（可选）
-- @return response, status_code, headers

function M.http_request(url, method, headers, body)
    local response = {}
    local parsed_url = url:match("^(https?)://")
    local use_https = (parsed_url == "https")
    
    local config = {
        url = url,
        method = method or "GET",
        headers = headers or {},
        sink = ltn12.sink.table(response)
    }
    
    if body then
        config.source = ltn12.source.string(body)
        config.headers["Content-Length"] = #body
    end
    
    local request_func = use_https and https.request or http.request
    local success, code, resp_headers = request_func(config)
    
    return table.concat(response), code or 500, resp_headers
end

return M -- 必须返回模块
