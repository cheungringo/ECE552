int main() {
	int i = 0;
	int a = 0;
	int b = 0;
	for(i = 0; i < 1000000; i++) {
		a = a + 1;
		b = a + 1;		
	}	
	return(b);
}
