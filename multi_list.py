def add_multi_list(list):
	sum = 0
	for el in list:
		if (type(el) == type(int())):
			sum = sum + el
		else if(type(el) == type(list())):
			sum = add_multi_list(el)

	return sum