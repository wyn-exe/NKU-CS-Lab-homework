for i in range(int(input())):
    n,d,h=map(int,input().split())
    c=0
    f=0
    A=list(map(int,input().split()))
    for a in A:
        if a==0:
            c=0 if c<d else c-d
        else:
            c=c+a

        if c>h:
            f=-1
            print('YES')
            break
    if f==0:
        print('NO')
