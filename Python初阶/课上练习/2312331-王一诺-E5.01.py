n=int(input())
sum=0
i=1
while i<=n:
    x,y=map(int,input().split())
    sum+=1 if y-x>=2 else 0
    i+=1
print(sum)
