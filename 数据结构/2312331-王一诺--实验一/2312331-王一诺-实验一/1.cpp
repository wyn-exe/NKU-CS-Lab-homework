#include<iostream>
using namespace std;

class Node
{
public:
	char data;
	Node* next;
	Node(char n)
	{
		data = n;
		next = NULL;
	}
};

class List
{
public:
	Node* head;
	int nodenum;
	List();
	~List();
	void Delete(int k);
	char Find(int k);
	void Print();
};

List::List()
{
	head = new Node(0);
	nodenum = 0;
}

List::~List()
{
	for (int i = nodenum; i >= 1; i--)
		Delete(i);
	delete head;
}

void List::Delete(int k)
{
	if (k<1 || k>nodenum)
		return;
	Node* p = head;
	for (int i = 1; i < k; i++)
		p = p->next;
	Node* q = p->next;
	p->next = q->next;
	delete q;
	nodenum--;
}

void List::Print()
{
	Node* p = head;
	while (p)
	{
		p = p->next;
		if (p != NULL && p->next != NULL)
			cout << p->data << " ";
		else if (p != NULL && p->next == NULL)
			cout << p->data << endl;
		else
			break;
	}
}

char List::Find(int k)
{
	if (k<1 || k>nodenum)
		throw invalid_argument("k²»ÕýÈ·");
	Node* p = head;
	for (int i = 1; i < k; i++)
		p = p->next;
	Node* q = p->next;
	return q->data;
}

int main()
{
	List* A = new List();
	List* B = new List();
	List* C = new List();
	List* D = new List();
	List* E = new List();
	char x;
	int k;
	cout << "A:";
	Node* p = A->head;
	while (cin >> x)
	{
		Node* newnode = new Node(x);
		p->next = newnode;
		p = p->next;
		A->nodenum++;

		if (cin.get() == '\n')
			break;
	}
	cout << "B:";
	Node* q = B->head;
	while (cin >> x)
	{
		Node* newnode = new Node(x);
		q->next = newnode;
		q = q->next;
		B->nodenum++;

		if (cin.get() == '\n')
			break;
	}
	cout << "k:";
	cin >> k;
	A->Print();
	B->Print();

	Node* r = A->head->next;
	Node* s = B->head->next;
	Node* a = r;
	Node* b = s;
	C->head->next = a;
	if (A->nodenum <= B->nodenum)
	{
		for (int i = 1; i < A->nodenum; i++)
		{
			r = r->next;
			a->next = b;
			s = s->next;
			a = r;
			b->next = a;
			b = s;
		}
		a->next = b;
	}
	else
	{
		for (int i = 1; i < B->nodenum; i++)
		{
			r = r->next;
			a->next = b;
			s = s->next;
			a = r;
			b->next = a;
			b = s;
		}
		r = r->next;
		a->next = b;
		b->next = r;
	}
	C->nodenum = A->nodenum + B->nodenum;
	C->Print();

	D->head->next = C->head->next;
	D->nodenum = C->nodenum;
	Node* d = D->head;
	int i = 1;
	while (d)
	{
		d = d->next;
		i++;
		for (int j = i; j <= D->nodenum; j++)
		{
			if (d->data == D->Find(j))
				D->Delete(j);
		}
	}
	D->Print();

	E->head->next = D->head->next;
	E->nodenum = D->nodenum;
	Node* e = E->head->next;
	Node* pre = E->head;
	int num = E->nodenum / k;
	if (num>0)
	{
		for (int i = 1; i <= num; i++)
		{
			int count = 1;
			while (e->next && count < k)
			{
				Node* current = e->next;
				e->next = current->next;
				current->next = pre->next;
				pre->next = current;
				count++;
			}
			pre = e;
			e = e->next;
		}
		E->Print();
	}
	else
		E->Print();

	return 0;
}