#include<iostream>
#include<math.h>
#include<string>
#include<sstream>
using namespace std;

//项栈//
class Node {
public:
	friend class ItemStack;
	float data = 0;//数字项
	string xstr;//未知数项
	float waiji = 1;//多项式外积
	bool isx = false;//用来记忆该节点存放的是否含有未知数
	Node* link;
	void copy(Node*& n);
	string out();
};
void Node::copy(Node*& n)
{
	data = n->data;
	xstr = n->xstr;
	waiji = n->waiji;
	isx = n->isx;
}
string Node::out()
{
	string n, s1;
	s1 = xstr.length() > 1 ? ("(" + xstr + ")") : xstr;
	if (data == 0)
	{
		stringstream ss;
		ss << waiji;
		n = waiji == 1 ? xstr : (ss.str() + "*" + s1);
	}
	else
	{
		stringstream ss;
		float WJ = waiji < 0 ? -waiji : waiji;
		ss << WJ;
		stringstream ss2;
		ss2 << (data * waiji);
		if(waiji>0)
			n = waiji == 1 ? (ss2.str() + "+" + xstr) : (ss2.str() + "+" + ss.str() + "*" + s1);
		else
			n = waiji == -1 ? (ss2.str() + "-" + xstr) : (ss2.str() + "-" + ss.str() + "*" + s1);
	}
	return n;
}
class ItemStack {
public:
	ItemStack() { top = 0; }
	~ItemStack();
	bool IsEmpty() const { return number == 0; }
	bool IsFull() const;
	int number = 0;
	float Top() const;
	ItemStack& Add(const float& x);//常数项压栈
	ItemStack& xAdd(char& x);//未知量压栈
	ItemStack& xAdd(float data, string& xstr, float waiji);//多项式压栈
	ItemStack& Delete(float& x);//删取数
	ItemStack& xDelete(float data, string& xstr, float waiji);//删取多项式或未知量
	Node* top;
};
ItemStack::~ItemStack()
{
	Node* next;
	while (top) {
		next = top->link;
		delete top;
		top = next;
	}
}
bool ItemStack::IsFull() const
{// Is the stack full?
	try {
		Node* p = new Node;
		delete p;
		return false;
	}
	catch (exception& e) { return true; }
}
float ItemStack::Top() const
{// Return top element.
	if (IsEmpty())
		throw "无效访问";
	return top->data;
}
ItemStack& ItemStack::Add(const float& x)
{
	Node* p = new Node;
	p->data = x;
	p->link = top;
	top = p;
	number++;
	return *this;
}
ItemStack& ItemStack::xAdd(char& x)
{
	Node* p = new Node;
	p->xstr += x;
	p->link = top;
	p->isx = true;
	top = p;
	number++;
	return *this;
}
ItemStack& ItemStack::xAdd(float data, string& xstr, float waiji)
{
	Node* p = new Node;
	p->xstr = xstr;
	p->data = data;
	p->waiji = waiji;
	p->link = top;
	p->isx = true;
	if (waiji == 0)
	{
		p->data = 0;
		p->isx = false;
	}
	top = p;
	number++;
	return *this;
}
ItemStack& ItemStack::Delete(float& x)
{
	x = top->data;
	Node* p = top;
	top = top->link;
	delete p;
	number--;
	return *this;
}
ItemStack& ItemStack::xDelete(float data, string& xstr, float waiji)
{
	xstr = top->xstr;
	data = top->data;
	waiji = top->waiji;
	Node* p = top;
	top = top->link;
	delete p;
	number--;
	return *this;
}




//运算符栈//
class node
{
public:
	friend class OperateStack;
	char data = 0;//运算符
	node* link;
};
class OperateStack {
public:
	OperateStack() { top = 0; }
	~OperateStack();
	bool IsEmpty() const { return number == 0; }
	bool IsFull() const;
	int number = 0;
	char Top() const;
	OperateStack& Add(const char& x);//添加
	OperateStack& Delete(char& x);//删取
	node* top;
};
OperateStack::~OperateStack()
{
	node* next;
	while (top) {
		next = top->link;
		delete top;
		top = next;
	}
}
bool OperateStack::IsFull() const
{// Is the stack full?
	try {
		node* p = new node;
		delete p;
		return false;
	}
	catch (exception& e) { return true; }
}
char OperateStack::Top() const
{// Return top element.
	if (IsEmpty())
		throw "无效访问";
	return top->data;
}
OperateStack& OperateStack::Add(const char& x)
{
	node* p = new node;
	p->data = x;
	p->link = top;
	top = p;
	number++;
	return *this;
}
OperateStack& OperateStack::Delete(char& x)
{
	x = top->data;
	node* p = top;
	top = top->link;
	delete p;
	number--;
	return *this;
}




//压栈函数//
void pushnumber(ItemStack*& stack1, float a, int& jishu)
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
void pushnumber(ItemStack*& stack1, char a)
{
	stack1->xAdd(a);
}
void pushnumber(ItemStack*& stack1, float data, string& xstr, float waiji)
{
	stack1->xAdd(data, xstr, waiji);
}
void pushoperate(OperateStack*& stack2, char a)
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
//pk两个操作符优先级//
int pk(OperateStack* op, char a)
{
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




//多项式运算以及结果传递//
void compute(float& outdata, string& outstr, float& outwaiji, Node* in1, Node* in2, char op)
{
	if (op == '+') {
		outdata = (in2->data * in2->waiji) + (in1->data * in1->waiji);
		outwaiji = 1;
		stringstream ss1; stringstream ss2; string s1, s2;
		float waiji1 = in1->waiji < 0 ? -in1->waiji : in1->waiji;
		ss1 << waiji1; ss2 << in2->waiji;
		if (in1->isx == true) {
			if (in1->waiji == 1)
				s1 = in1->xstr;
			else
				s1 = in1->xstr.length() > 1 ? (ss1.str() + "*(" + in1->xstr + ")") : (ss1.str() + "*" + in1->xstr);
		}
		else
			s1 = "";
		if (in2->isx == true) {
			if (in2->waiji == 1)
				s2 = in2->xstr;
			else
				s2 = in2->xstr.length() > 1 ? (ss2.str() + "*(" + in2->xstr + ")") : (ss2.str() + "*" + in2->xstr);

			if (s1 == "")
				outstr = s2;
			else
				outstr = in1->waiji<0? s2 + "-" + s1:s2 + "+" + s1;
		}
		else
		{
			if (in1->waiji < 0)
			{
				outwaiji = -1;
				outdata = -outdata;
			}
			outstr = s1;
		}
	}
	if (op == '-') {
		outdata = (in2->data * in2->waiji) - (in1->data * in1->waiji);
		stringstream ss1; stringstream ss2; string s1, s2;
		float waiji1 = in1->waiji < 0 ? -in1->waiji : in1->waiji;
		float waiji2 = in2->waiji < 0 ? -in2->waiji : in2->waiji;
		ss1 << waiji1; ss2 << waiji2;
		if (in1->isx == true) {
			if (in1->waiji == 1)
				s1 = in1->xstr.length() > 1 ? ("(" + in1->xstr + ")") : (in1->xstr);
			else
				s1 = in1->xstr.length() > 1 ? (ss1.str() + "*(" + in1->xstr + ")") : (ss1.str() + "*" + in1->xstr);
		}
		else
			s1 = "";
		if (in2->isx == true) {
			outwaiji = 1;
			if (in2->waiji == 1)
				s2 = in2->xstr.length() > 1 ? ("(" + in2->xstr + ")") : (in2->xstr);
			else
			{
				if (in2->waiji < 0)
				{
					outdata = -outdata;
					outwaiji = -1;
				}
				s2 = in2->xstr.length() > 1 ? (ss2.str() + "*(" + in2->xstr + ")") : (ss2.str() + "*" + in2->xstr);
			}

			if (s1 == "")
				outstr = s2;
			else
			{
				if (in2->waiji < 0)
					outstr = in1->waiji < 0 ? (s2 + "-" + s1) : (s2 + "+" + s1);
				else
					outstr = in1->waiji < 0 ? (s2 + "+" + s1) : (s2 + "-" + s1);
			}
		}
		else
		{
			outdata = -outdata;
			outwaiji = -1;
			outstr = s1;
		}
	}
	if (op == '*') {
		if (in2->isx == false)
		{
			outdata = in1->data;
			outwaiji = (in2->data * in2->waiji) * in1->waiji;
			outstr = in1->xstr;
		}
		else
		{
			if (in1->isx == false)
			{
				outdata = in2->data;
				outwaiji = (in1->data * in1->waiji) * in2->waiji;
				outstr = in2->xstr;
			}
			else
			{
				outdata = in2->data * in1->data;
				outwaiji = in2->waiji * in1->waiji;
				stringstream ss1; stringstream ss2; string s1, s2,S1,S2,sss1,sss2;
				float data1 = in1->data < 0 ? -in1->data : in1->data;
				ss1 << data1; ss2 << in2->data;
				s1 = (in1->xstr.length() > 1&& in2->data != 1 )? ("(" + in1->xstr + ")") : (in1->xstr);
				s2 = (in2->xstr.length() > 1 && in1->data != 1)? ("(" + in2->xstr + ")") : (in2->xstr);
				if (in1->data < 0)
				{
					S1 = in1->data == 0 ? "" : ss1.str() + "*" + s2 + "+";
					S2 = in2->data == 0 ? "" : ss2.str() + "*" + s1 + "-";
				}
				else
				{
					S1 = in1->data == 0 ? "" : ss1.str() + "*" + s2 + "+";
					S2 = in2->data == 0 ? "" : ss2.str() + "*" + s1 + "+";
				}
				sss1 = in1->xstr.length() > 1 ? ("(" + in1->xstr + ")") : (in1->xstr);
				sss2 = in2->xstr.length() > 1  ? ("(" + in2->xstr + ")") : (in2->xstr);
				outstr = S2 + S1 + sss2 + "*" + sss1;
			}
		}
	}
	if (op == '/') {
		if (in1->waiji == 0 || (in1->isx == false && in1->data == 0)) { throw "分母不能为0"; }
		else
		{
			if (in1->isx == false)
			{
				outdata = in2->data;
				outwaiji = in2->waiji / in1->data;
				outstr = in2->xstr;
			}
			else
			{
				string s1 = in1->out().length() > 1 ? ("(" + in1->out() + ")") : in1->out();
				if (in2->isx == false)
				{
					stringstream ss2;
					ss2 << in2->data;
					outdata = 0;
					outwaiji = 1;
					outstr = ss2.str() + "/" + s1;
				}
				else
				{
					outdata = 0;
					outwaiji = 1;
					string s2 = in2->out().length() > 1 ? ("(" + in2->out() + ")") : in2->out();
					outstr = s2 + "/" + s1;
				}
			}
		}
	}
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
	a[length] = '#';//末尾压入#确保运算进行到底
	ItemStack* numbernode = new ItemStack; OperateStack* operatenode = new OperateStack;
	pushoperate(operatenode, '#');//先把一个#压进操作符栈
	for (int i = 0; i < length + 1; i++)
	{
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
				char op;//储存运算符
				Node* in1 = new Node; Node* in2 = new Node;//储存要运算的两项
				operatenode->Delete(op);
				try {
					if (numbernode->IsEmpty())
						throw "False";
					else
					{
						in1->copy(numbernode->top);
						numbernode->Delete(a1);
					}
				}
				catch (const char* error)
				{
					cerr << error << endl;
					return 0;
				}
				try {
					if (numbernode->IsEmpty())
						throw "False";
					else
					{
						in2->copy(numbernode->top);
						numbernode->Delete(a2);
					}
				}
				catch (const char* error)
				{
					cerr << error << endl;
					return 0;
				}
				
				if (in1->isx == false && in2->isx == false)
				{
					if (op == '+') {
						pushnumber(numbernode, a2 + a1, jishu);
					}
					if (op == '-') {
						pushnumber(numbernode, a2 - a1, jishu);
					}
					if (op == '*') {
						pushnumber(numbernode, a2 * a1, jishu);
					}
					if (op == '/') {
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
					float outdata; string outstr; float outwaiji;
					compute(outdata, outstr, outwaiji, in1, in2, op);
					pushnumber(numbernode, outdata, outstr, outwaiji);
				}
				i--;
			}
			if (x == 0) {//如果是括号匹配或者#匹配则删去
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
		string outstr=numbernode->top->out();
		float out;
		numbernode->Delete(out);
		if (numbernode->IsEmpty() == true)
		{
			cout << "True" << endl;
			cout << outstr << endl;
		}
		else
		{
			cout << "False" << endl;
			return 0;
		}

		//优化(再来一遍就行)//
		char b[1000];
		int len = 0;
		for (char ch : outstr)
		{
			b[len] = ch;
			len++;
		}
		b[len] = '#';
		jishu = 0;
		pushoperate(operatenode, '#');
		for (int i = 0; i < len + 1; i++)
		{
			if (b[i] == '+' || b[i] == '-' || b[i] == '*' || b[i] == '/' || b[i] == '(' ||
				b[i] == ')' || b[i] == '#') {
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
					char op;
					operatenode->Delete(op);
					Node* in1 = new Node; Node* in2 = new Node;
					in1->copy(numbernode->top); numbernode->Delete(a1);
					in2->copy(numbernode->top); numbernode->Delete(a2);

					if (in1->isx == false && in2->isx == false)
					{
						if (op == '+') {
							pushnumber(numbernode, a2 + a1, jishu);
						}
						if (op == '-') {
							pushnumber(numbernode, a2 - a1, jishu);
						}
						if (op == '*') {
							pushnumber(numbernode, a2 * a1, jishu);
						}
						if (op == '/') {
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
						float outdata; string outstr; float outwaiji;
						compute(outdata, outstr, outwaiji, in1, in2, op);
						pushnumber(numbernode, outdata, outstr, outwaiji);
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
				pushnumber(numbernode, b[i] - '0', jishu);
			}
			else if ((b[i] >= 'a' && b[i] <= 'z') || b[i] >= 'A' && b[i] <= 'Z')
			{
				pushnumber(numbernode, b[i]);
			}
			else
			{
				cout << "False" << endl;
				return 0;
			}
		}

		cout << numbernode->top->out() << endl;
	}

	return 0;
}