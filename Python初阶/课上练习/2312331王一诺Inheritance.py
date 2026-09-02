class Person:
    def __init__(self,name="",gender=""):
        self.name=name
        self.gender=gender
    def setname(self,name):
        self.name=name
    def setgender(self,gender):
        self.gender=gender
    def __str__(self):
        return f"名字：{self.name}，性别：{self.gender}"

class Student(Person):
    def __init__(self,sno=0,major=""):
        self.sno=sno
        self.major=major
    def setsno(self,sno):
        self.sno=sno
    def setmajor(self,major):
        self.major=major
    def __str__(self):
        return f"名字：{self.name}，性别：{self.gender}，学号：{self.sno}，专业：{self.major}"

class Teacher(Person):
    def __init__(self,tno=0,depart=""):
        self.tno=tno
        self.depart=depart
    def settno(self,tno):
        self.tno=tno
    def setdepart(self,depart):
        self.depart=depart
    def __str__(self):
        return f"名字：{self.name}，性别：{self.gender}，工号：{self.tno}，部门：{self.depart}"

class TA(Student,Teacher):
    def __init__(self,teacher=""):
        self.teacher=teacher
    def setteacher(self,teacher):
        self.teacher=teacher
    def __str__(self):
        return f"名字：{self.name}，性别：{self.gender}，学号：{self.sno}，部门：{self.depart}，老师：{self.teacher}"

if __name__=='__main__':
    #person对象
    p=Person('小蓝','男')
    print(p)
    #student对象
    s=Student('2310007','计科')
    s.setname('小绿')
    s.setgender('女')
    print(s)
    #teacher对象
    t=Teacher()
    t.settno('012002')
    t.setdepart('计算机学院')
    t.setname('陈老师')
    t.setgender('男')
    print(t)
    #TA对象
    ta=TA('陈老师')
    ta.setname('孙助教')
    ta.setgender('男')
    ta.setsno('2120230613')
    ta.setdepart('计算机学院')
    print(ta)
