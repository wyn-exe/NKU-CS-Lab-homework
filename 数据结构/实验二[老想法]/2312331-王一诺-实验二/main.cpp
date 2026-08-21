#include<iostream>
#include<math.h>
#include<string>
#include<sstream>
using namespace std;

template <class T>
class Node {
public:
	friend T;
	T data;
	string xstr;//用来存放未知数
	bool isx = false;//用来记忆该节点存放的是不是未知数
	Node<T>* link;
};
template <class T>
class LinkedStack {
public:
	LinkedStack() { top = 0; }
	~LinkedStack();
	bool IsEmpty() const { return top == 0; }
	bool IsFull() const;
	T Top() const;
	LinkedStack <T>& Add(const T& x);
	LinkedStack <T>& xAdd(string& x);
	LinkedStack <T>& xAdd(char& x);
	LinkedStack <T>& Delete(T& x);
	LinkedStack <T>& xDelete(string& x);
	Node<T>* top; // pointer to top node
};

template<class T>
LinkedStack<T>::~LinkedStack()
{// Stack destructor..
	Node<T>* next;
	while (top) {
		next = top->link;
		delete top;
		top = next;
	}
}
template<class T>
bool LinkedStack<T>::IsFull() const
{// Is the stack full?
	try {
		Node<T>* p = new Node<T>;
		delete p;
		return false;
	}
	catch (exception& e) { return true; }
}
template<class T>
T LinkedStack<T>::Top() const
{// Return top element.
	if (IsEmpty())
		throw "无效访问";
	return top->data;
}
template<class T>
LinkedStack<T>& LinkedStack<T>::Add(const T& x)
{// Add x to stack.
	Node<T>* p = new Node<T>;
	p->data = x;
	p->link = top;
	top = p;
	return *this;
}
template<class T>
LinkedStack<T>& LinkedStack<T>::xAdd(string& x)
{// Add 未知量x to stack.
	Node<T>* p = new Node<T>;
	p->xstr = x;
	p->link = top;
	p->isx = true;
	top = p;
	return *this;
}
template<class T>
LinkedStack<T>& LinkedStack<T>::xAdd(char& x)
{// Add 未知量x to stack.
	Node<T>* p = new Node<T>;
	p->xstr += x;
	p->link = top;
	p->isx = true;
	top = p;
	return *this;
}
template<class T>
LinkedStack<T>& LinkedStack<T>::Delete(T& x)
{// Delete top element and put it in x.
	if (IsEmpty()) throw "无效访问";
	x = top->data;
	Node<T>* p = top;
	top = top->link;
	delete p;
	return *this;
}
template<class T>
LinkedStack<T>& LinkedStack<T>::xDelete(string& x)
{// Delete top element and put it in x.
	if (IsEmpty()) throw "无效访问";
	x = top->xstr;
	Node<T>* p = top;
	top = top->link;
	delete p;
	return *this;
}

//压栈函数//
void pushnumber(LinkedStack<float>*& stack1, float a, int& jishu)
{
	float result = 0;
	if (jishu == 0)
	{
		stack1->Add(a);
		jishu++;
	}
	else if (jishu <= -1)
	{
		result = (stack1->Top()) + a * (pow(10, jishu));
		stack1->top->data = result;
		jishu--;
	}
	else
	{
		result = (stack1->Top()) * 10 + a;
		stack1->top->data = result;
	}
}
void pushnumber(LinkedStack<float>*& stack1, string a)
{
	stack1->xAdd(a);
}
void pushnumber(LinkedStack<float>*& stack1, char a)
{
	stack1->xAdd(a);
}
void pushoperate(LinkedStack<char>*& stack2, char a)
{
	stack2->Add(a);
}


//定义优先级//
int check(char a) {
	if (a == '+') {
		return 0;
	}
	if (a == '-') {
		return 1;
	}
	if (a == '*') {
		return 2;
	}
	if (a == '/') {
		return 3;
	}
	if (a == '(') {
		return 4;
	}
	if (a == ')') {
		return 5;
	}
	if (a == '#') {
		return 6;
	}
}
int pk(LinkedStack<char>* op, char a) {//pk两个操作符优先级
	int x[7][7] = { -1,-1, 1, 1, 1,-1,-1,
		            -1,-1, 1, 1, 1,-1,-1,
		            -1,-1,-1,-1, 1,-1,-1,
		            -1,-1,-1,-1, 1,-1,-1 ,
		             1, 1, 1, 1, 1, 0, 2,
		            -1,-1,-1,-1, 2,-1,-1,
		             1, 1, 1, 1, 1, 2, 0 };
	//1表示大于，0表示相等，-1表示小于,2表示表达式错误
	return x[check(op->top->data)][check(a)];
}

int main()
{
	char a[1000];
	char n;
	int length = 0; int jishu = 0;
	while (cin.get(n))
	{
		if (n == '\n')
			break;
		a[length] = n;
		length++;

	}
	a[length] = '#';
	LinkedStack<float>* numbernode = new LinkedStack<float>; LinkedStack<char>* operatenode = new LinkedStack<char>;
	pushoperate(operatenode, '#');//先把一个#压进操作符栈
	for (int i = 0; i < length + 1; i++) {
		if (a[i] == '+' || a[i] == '-' || a[i] == '*' || a[i] == '/' || a[i] == '(' ||
			a[i] == ')' || a[i] == '#') {//如果为操作符
			jishu = 0;
			int x = pk(operatenode, a[i]);//优先级比较的结果放进x
			if (x == 2) {//如果优先级比较出错，则断开
				cout << "False" << endl;
				return 0;
			}
			if (x == 1) {//如果“要压的操作符的优先级”大于“栈顶操作符的优先级”，则正常压栈
				pushoperate(operatenode, a[i]);
			}
			if (x == -1) {//如果“要压的操作符的优先级”小于“栈顶操作符的优先级”，则：
				float a1 = 0; float a2 = 0;//储存要运算的两个数据
				string b1, b2;//储存未知量
				char a3;//储存运算符
				operatenode->Delete(a3);
				if (numbernode->top->isx == false)//对要运算的两个数是否含未知量展开分类讨论
				{
					numbernode->Delete(a1);
					if (numbernode->top->isx == false)
					{
						numbernode->Delete(a2);
						if (a3 == '+') {
							pushnumber(numbernode, a2 + a1, jishu);
						}
						if (a3 == '-') {
							pushnumber(numbernode, a2 - a1, jishu);
						}
						if (a3 == '*') {
							pushnumber(numbernode, a2 * a1, jishu);
						}
						if (a3 == '/') {
							if (a1 == 0)
							{
								cout << "分母不能为0" << endl;
								return 0;
							}
							else
								pushnumber(numbernode, a2 / a1, jishu);
						}
					}
					else
					{
						if (a1 == 0 && a3 == '/')
						{
							cout << "分母不能为0" << endl;
							return 0;
						}
						stringstream ss1;
						ss1 << a1;
						numbernode->xDelete(b2);
						if ((a3 == '*' || a3 == '/') && b2.length() > 1)
						{
							b2 = "(" + b2 + ")";
						}
						string m = b2 + a3 + ss1.str();
						pushnumber(numbernode, m);
					}
				}
				else
				{
					numbernode->xDelete(b1);
					if (numbernode->top->isx == false)
					{
						if ((a3 == '*' || a3 == '/') && b1.length() > 1)
						{
							b1 = "(" + b1 + ")";
						}
						numbernode->Delete(a2);
						stringstream ss2;
						ss2 << a2;
						string m = ss2.str() + a3 + b1;
						pushnumber(numbernode, m);
					}
					else
					{
						if ((a3 == '*' || a3 == '/') && b1.length() > 1)
						{
							b1 = "(" + b1 + ")";
						}
						if ((a3 == '*' || a3 == '/') && b2.length() > 1)
						{
							b2 = "(" + b2 + ")";
						}
						numbernode->xDelete(b2);
						string m = b2 + a3 + b1;
						pushnumber(numbernode, m);
					}
				}
				i--;
			}
			if (x == 0) {
				char a3;
				operatenode->Delete(a3);
			}
		}
		else if (a[i] == '.')
		{
			jishu = -1;
		}
		else if (a[i] >= '0' && a[i] <= '9')//数字栈
		{
			pushnumber(numbernode, a[i] - '0', jishu);
		}
		else if ((a[i] >= 'a' && a[i] <= 'z') || a[i] >= 'A' && a[i] <= 'Z')//未知量入栈
		{
			pushnumber(numbernode, a[i]);
		}
		else
		{
			cout << "False" << endl;
			return 0;
		}
	}


	if (operatenode->top != 0)
	{
		cout << "False" << endl;
		return 0;
	}


	if (numbernode->top->isx == false)
	{
		float out;
		numbernode->Delete(out);
		if (numbernode->IsEmpty() == true)
		{
			cout << "True" << endl;
			cout << out << endl;
		}
		else
		{
			cout << "False" << endl;
			return 0;
		}
	}
	else
	{
		string out;
		numbernode->xDelete(out);
		if (numbernode->IsEmpty() == true)
		{
			cout << "True" << endl;
			cout << out << endl;
		}
		else
		{
			cout << "False" << endl;
			return 0;
		}

		//进行优化操作
		LinkedStack<float>* youhua = new LinkedStack<float>;
		char b[1000];
		int len = 0;
		for (char ch : out)
		{
			b[len] = ch;
			len++;
		}
		b[len] = '#';
		jishu = 0;
		//再进行一次入出栈，但这次在过程中处理0、1、负数和绝对值
		pushoperate(operatenode, '#');
		for (int i = 0; i < len + 1; i++) {
			if (b[i] == '+' || b[i] == '-' || b[i] == '*' || b[i] == '/' || b[i] == '(' ||
				b[i] == ')' || b[i] == '#') {//如果为操作符
				jishu = 0;
				int x = pk(operatenode, b[i]);
				if (x == 2) {
					cout << "False" << endl;
					return 0;
				}
				if (x == 1) {
					pushoperate(operatenode, b[i]);
				}
				if (x == -1) {
					float a1 = 0; float a2 = 0;
					string b1, b2;
					char a3;
					operatenode->Delete(a3);
					//对要运算的两个数是否含未知量展开分类讨论
					if (youhua->top->isx == false)
					{
						youhua->Delete(a1);
						if (youhua->top->isx == false)
						{
							youhua->Delete(a2);
							if (a3 == '+') {
								pushnumber(youhua, a2 + a1, jishu);
							}
							if (a3 == '-') {
								pushnumber(youhua, a2 - a1, jishu);
							}
							if (a3 == '*') {
								pushnumber(youhua, a2 * a1, jishu);
							}
							if (a3 == '/') {
								if (a1 == 0)
								{
									cout << "分母不能为0" << endl;
									return 0;
								}
								else
									pushnumber(youhua, a2 / a1, jishu);
							}
						}
						else
						{
							stringstream ss1;
							ss1 << a1;
							youhua->xDelete(b2);
							if (a1 == 0)
							{
								if (a3 == '+' || a3 == '-')
								{
									string m = b2;
									pushnumber(youhua, m);
								}
								else if (a3 == '/')
								{
									cout << "分母不能为0" << endl;
									return 0;
								}
								else
									pushnumber(youhua,0,jishu);
							}
							else if (a1 == 1 && (a3 == '*' || a3 == '/'))
							{
								pushnumber(youhua, b2);
							}
							else if ((a3 == '*' || a3 == '/') && b2.length() > 1)
							{
								b2 = "(" + b2 + ")";
								string m = b2 + a3 + ss1.str();
								pushnumber(youhua, m);
							}
							else
							{
								string m = b2 + a3 + ss1.str();
								pushnumber(youhua, m);
							}
						}
					}
					else
					{
						youhua->xDelete(b1);
						if (youhua->top->isx == false)
						{
							youhua->Delete(a2);
							stringstream ss2;
							ss2 << a2;
							if (a2 == 0)
							{
								if (a3 == '+')
								{
									string m = b1;
									pushnumber(youhua, m);
								}
								if (a3 == '-')
								{
									string m = "-" + b1;
									pushnumber(youhua, m);
								}
								else
									pushnumber(youhua, 0, jishu);
							}
							else if (a2 == 1 && a3 == '*')
							{
								pushnumber(youhua, b1);
							}
							else if ((a3 == '*' || a3 == '/') && b1.length() > 1)
							{
								b1 = "(" + b1 + ")";
								string m = ss2.str() + a3 + b1;
								pushnumber(youhua, m);
							}
							else
							{
								string m = ss2.str() + a3 + b1;
								pushnumber(youhua, m);
							}
						}
						else
						{
							youhua->xDelete(b2);
							if (a3 == '-' && b1 == b2) { pushnumber(youhua, 0, jishu); }
							else if (a3 == '/' && b1 == b2)
							{
								float x = 1;
								pushnumber(youhua,x, jishu); 
							}
							else
							{
								if ((a3 == '*' || a3 == '/') && b1.length() > 1)
								{
									b1 = "(" + b1 + ")";
								}
								if ((a3 == '*' || a3 == '/') && b2.length() > 1)
								{
									b2 = "(" + b2 + ")";
								}
								string m = b2 + a3 + b1;
								pushnumber(youhua, m);
							}
						}
					}
					i--;
				}
				if (x == 0) {
					char a3;
					operatenode->Delete(a3);
				}
			}
			else if (b[i] == '.')
			{
				jishu = -1;
			}
			else if (b[i] >= '0' && b[i] <= '9')
			{
				pushnumber(youhua, b[i] - '0', jishu);
			}
			else if ((b[i] >= 'a' && b[i] <= 'z') || b[i] >= 'A' && b[i] <= 'Z')
			{
				pushnumber(youhua, b[i]);
			}
			else
			{
				cout << "False" << endl;
				return 0;
			}
		}
		if (youhua->top->isx == false)
		{
			cout << youhua->top->data << endl;
		}
		else
			cout << youhua->top->xstr;
	}
	return 0;
}