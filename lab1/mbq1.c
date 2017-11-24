int main() {
	int i = 0;
	int a = 0;
	int b = 0;
	for(i = 0; i < 1000000; i++) {
		a = a + 1;
		b = a + 1;
		__asm__("lw $8, 16($sp)");
	        __asm__("addi $7, $8, 1");
		__asm__("sw $7, 16($sp)");
		/* generated assembly code 
			addu	$4,$4,1  	
			addu	$5,$4,1    // 2-cycle stall q1, 1-cycle stall q2
	        	lw      $8,16($sp) 
                	addi    $7,$8,1    // 2-cycle stall q1, 2-cycle stall q2
			sw	$7,16($sp) // 2-cycle stall q1, no stall q2     
                     	addu	$3,$3,1  
                	slt	$2,$6,$3   // 2-cycle stall q1, 1-cycle stall q2
                	beq	$2,$0,$L5  // 2-cycle stall q1, 1-cycle stall q2
		* end generated assembly code */
	}
	return(b);
}	
