
local function get_script_dir()
    -- 获取当前脚本的绝对路径（兼容 require 和直接执行）
    local source = debug.getinfo(2, "S").source:sub(2)
    local script_path = source:match("^@(.*)") or source  -- 处理 require 和直接执行的区别
    return script_path:match("(.*[/\\])") or "./"  -- 提取目录部分
end

-- 动态添加到 package.path
local script_dir = get_script_dir()
package.path = package.path .. ";" .. script_dir .. "?.lua"

-- 引入同一个目录下的文件
local http_post = require("http_post")

code, body, header = http_post.http_request("http://localhost:8002/api?call=shell&wait=0.1", "POST", { ["User-Agent"] = "Lua-cURL" })

print("Status:", code)
print("Body:", body)
