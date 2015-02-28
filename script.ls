# This is a test script

extern int foo(int a, float b, string c, bool d)

int test(int a, float b, string c, bool d)
{
	foo((1+1), b, c, d);
	2.+6.;
	4;
}

int caller() 
{
	test(1, 2.5, "abc", true);
}

bool init()
{
	true;
}

void exit()
{

}
