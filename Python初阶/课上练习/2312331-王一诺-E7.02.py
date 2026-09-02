n=int(input())
a=[int(i) for i in list(input())]
b=[int(i) for i in list(input())]
print(sum([min(abs(x-y),10-abs(x-y)) for x,y in zip(a,b)]))
