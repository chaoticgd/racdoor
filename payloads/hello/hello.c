volatile int a = 0;

void racdoor_entry(void)
{
	while(a != 123);
}
