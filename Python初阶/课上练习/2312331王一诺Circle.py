class Circle:
    def __init__(self,radius=5):
        print('构造方法被调用!')
        self.radius=radius
    def setr(self,r):
        self.radius=r
    def getPerimeter(self):
        print('周长:%f'%(self.radius*6.28))
    def getArea(self):
        print('面积:%f'%(self.radius*self.radius*3.14))

if __name__=='__main__':
    c1=Circle()
    c1.getPerimeter()
    c1.getArea()
    c1.setr(10)
    c1.getPerimeter()
    c1.getArea()
    c2=Circle(2)
    c2.getPerimeter()
    c2.getArea()
