for _ in range(int(input())):
    a,b=map(int,input().split())
    print('YES' if
          abs(a-b)==2 or (abs(a-b)==1 and min(a,b)%2==1)
          else 'NO')
