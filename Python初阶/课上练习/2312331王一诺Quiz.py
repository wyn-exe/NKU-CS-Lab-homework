class Account:
    def __init__(self,idset=0,balance=100,annualInterestRate=0):
        print("构造函数被调用")
        self.__id=idset
        self.__balance=balance
        self.__annualInterestRate=annualInterestRate
    def getid(self):
        print("id:%d"%(self.__id))
    def idset(self,idset=0):
        self.__id=idset
    def getbalance(self):
        print("balance:%f"%(self.__balance))
    def balanceset(self,balanceset=100):
        self.__balance=balanceset
    def getannualInterestRate(self):
        print("annualInterestRate:%f"%(self.__annualInterestRate))
    def annualInterestRateset(self,annualInterestRateset=0):
        self.__annualInterestRate=annualInterestRateset

    def withdraw(self,num=0):
        if (num<0)|(num>self.__balance):
            print("取款失败")
        else:
            self.__balance=self.__balance-num
            print("取款成功，取出：%f,当前余额：%f"%(num,self.__balance))
    def deposit(self,num=0):
        if num<0:
            print("存款失败")
        else:
            self.__balance=self.__balance+num
            print("存款成功，存入：%f,当前余额：%f"%(num,self.__balance))

if __name__=='__main__':
    a1=Account()
    a1.getid()
    a1.getbalance()
    a1.getannualInterestRate()
    a1.idset(1)
    a1.balanceset(5000)
    a1.annualInterestRateset(2024)
    a1.getid()
    a1.getbalance()
    a1.getannualInterestRate()
    a1.withdraw(6000)
    a1.deposit(1000)
    a1.withdraw(6000)
