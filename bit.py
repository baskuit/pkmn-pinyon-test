import sys
x = int(sys.argv[1])
print("x: ", x)
s = ""
i = 0
min = 0
while x or (min > i):
    s = str(x % 2) + s
    x //= 2
    i += 1
    if (i % 8 == 0):
        s = "," + s
print(s)    