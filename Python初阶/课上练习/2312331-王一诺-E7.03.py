for _ in range(int(input())):
    n=int(input())
    l=list(map(int,input().split()))
    k=int(input())
    v=l[k-1]
    l.sort()
    print(l.index(v)+1)
