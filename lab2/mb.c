int main(){
	int a, i;
	for (i=0;i<100000;i++){
		if (i%8  ==0){
			a = 10;
		}
		a = 15;
	}
	return 0;
/*.L4:	
	...
# Branch taken ((100000-1)/x) number of times
	jne	.L3
	movl	$10, -8(%rbp)
# Moving in literal 15 into a, incrementing i
.L3:
	movl	$15, -8(%rbp)
	add	$1, -4(%rbp)
# Checking end of loop
.L2:
	cmpl	$99999, -4(%rbp)
	jle 	.L4
	...*/
}
