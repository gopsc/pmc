-- 定义一个计算阶乘的函数
function fact (n)
	if n == 0 then
		return 1
	else
		return n * fact(n - 1)
	end
end

print("enter a number: ")
a = io.read("*n")  -- 读取一个数字
print(fact(a))
