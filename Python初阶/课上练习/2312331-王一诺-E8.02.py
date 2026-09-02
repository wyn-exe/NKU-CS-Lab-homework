a=[input() for _ in range(int(input()))]
b=set(a)
if len(b)==2:
    i,j=b
    print(i if a.count(i)>a.count(j) else j)
else:
    print(a[0])
