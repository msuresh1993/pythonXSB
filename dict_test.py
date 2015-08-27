
def init_dict():
	dictq = {}
	return dictq;
def add_dict(dictq, key, value):
	if type(dictq) == type(dict()):
		dictq[key] = value
	return 1

def get_value(dictq, key):
	if key in dictq.keys():
		return dictq[key]
