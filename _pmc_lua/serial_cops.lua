#!/usr/bin/lua
local socket = require("socket")
local posix = require("posix")
--local fcntl = require("posix.fcntl")
local argparse = require("argparse")

----------------------------------------
----------------------------------------
-- 打开串口
local function openSerial(port, baudrate)
	-- 设置串口参数
	local tty = posix.open(port, posix.O_RDWR + posix.O_NOCTTY)-- + fcntl.O_NDELAY)
	if not tty then
		return nil, "无法打开串口"
	end

	-- 配置串口
	local termios = posix.tcgetattr(tty)
	termios.c_iflag = 0
	termios.c_oflag = 0
	termios.c_cflag = posix.CS8 + posix.CREAD + posix.CLOCAL
	termios.c_lfalg = 0
	--termios.c_cc[1] = 1 -- VMIN
	--termios.c_cc[2] = 0 -- VTIME

	-- 设置波特率
	local baud_map = {
		[50] = posix.B50,
		[75] = posix.B75,
		[110] = posix.B110,
		[134] = posix.B134,
		[150] = posix.B150,
		[200] = posix.B200,
		[300] = posix.B300,
		[600] = posix.B600,
		[1200] = posix.B1200,
		[1800] = posix.B1800,
		[2400] = posix.B2400,
		[4800] = posix.B4800,
		[9600] = posix.B9600,
		[19200] = posix.B19200,
		[38400] = posix.B38400,
		[57600] = posix.B57600,
		[115200] = posix.B115200
	}

	termios.c_ispeed = baud_map[baudrate] or posix.B9600
	termios.c_ospeed = termios.c_ispeed

	posix.tcsetattr(tty, posix.TCSANOW, termios)
	posix.fcntl(tty, posix.F_SETFL, 0) -- 清除非阻塞标志

	return tty
end

----------------------------------------
local function cops_ser_comm(device, command, wait)
	-- 打开串口
	local tty = openSerial(device, 115200)
	if not tty then
		print("打开串口失败")
		return ""
	end
	-- 发送数据
	posix.write(tty, command .. "\n");
	-- 等待对方处理
	socket.sleep(tonumber(wait))
	-- 接收数据
	local data = posix.read(tty, 1024)
	-- 关闭串口
	posix.close(tty)
	return data
end

----------------------------------------
----------------------------------------
local parser = argparse("cops serial", "cops serial communication")
parser:argument("dev", "target serial device")
parser:option("--cmd", "targeet executable command")
parser:option("--wait", "second to wait")

local args = parser:parse()
print(cops_ser_comm(args.dev, args.cmd, args.wait))
