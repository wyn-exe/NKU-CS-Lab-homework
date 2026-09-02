for i in range(int(input())):
    s={chr(i+ord("a")) for i in range(26)}
    for _ in range(int(input())):
        s=s.intersection(set(input()))

    print(len(s))
