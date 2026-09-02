for i in range(int(input())):
    s=input()
    strlist = s.split()
    for j in range(len(strlist)-1):
        strlist[j] = strlist[j][0].upper() + '.'
    strlist[-1] = strlist[-1][0].upper() + strlist[-1][1:].lower()
    print("".join(strlist))
