while (iter > 1) {
	iter -= 1;
	if (iter-1 == 0) {
		j=0;
	} else {
		j=rand() % (iter -1;
	}
			
//	int j = iter - 1 == 0 ? 0 : rand() % (iter - 1); */
	int64_t tmp = A[iter];
	A[iter] = A[j];
	A[j] = tmp;
}

