for _ in range(int(input())):
    n=int(input())
    s=list(input())
    a=len(s)
    if a%2==0:
        a-=1

    for i in range(1,a,2):
        s[i],s[i+1]=s[i+1],s[i]

    for i in range(len(s)):
        s[i]=chr(97+(122-ord(s[i])))

    print("".join(s))
               
               
