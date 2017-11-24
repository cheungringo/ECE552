#include <stdio.h>

int main(){
	int a[5][5] = {1};
	int i = 0, j = 0;
	
	/*for (;i<5;i++){
		for (;j<5;j++){
			printf("%d ", a[i][j]);
		}
		printf("\n");
	}*/
	int b[5] = {1};
	for (;i<5;i++){
		printf("%d ", ++b[i]);
		printf("%d ", b[i]);
	}
	return 0;
}
