dic = {}
pos = {}
k = 0
for i in range(int(input())):
    name,chr_score = input().split()
    score = int(chr_score)
    k += 1
    pos[name] = k

    dic[name] = dic.get(name, 0) + score

m = max(dic.values())
d2 = {x for x,y in dic.items() if y == m}

print(min(d2,key=pos.get))

