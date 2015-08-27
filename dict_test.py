def init_dict():
	dict = {}
	return dict;
def add_dict(dict, key, value):

	dict[key] = value
	print("python priny", dict[key])
	return 1

def get_value(dict, key):
	if key in dict.keys():
		return dict[key]
dict = init_dict()
add_dict(dict, "hello", "world")
