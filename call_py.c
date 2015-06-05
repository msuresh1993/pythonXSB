//author: Muthukumar Suresh muthukumar5393@gmail.com
#include<math.h>
#include<stdio.h>
#include<string.h>
//#include<alloca.h>
#include<python2.7/Python.h>
#include<cinterf.h>
size_t ref_id_counter = 1; //the reference id that is passed to the prolog side
struct pyobj_ref_node
{
	PyObject *python_obj;
	size_t ref_id;
	struct pyobj_ref_node *next;
}; // mapping of the python object to the reference id passed to the prolog side. 
typedef struct pyobj_ref_node pyobj_ref_node;

pyobj_ref_node *pyobj_ref_list = NULL;// linked list to keep all the mappings 

size_t get_next_ref_id()
{
	size_t return_val = ref_id_counter;
	ref_id_counter++;
	return return_val;
}// generates a new id

//this function makes a new mapping node
pyobj_ref_node *make_pyobj_ref_node(PyObject *pynode)
{
	pyobj_ref_node *node = malloc(sizeof(pyobj_ref_node));
	node->python_obj = pynode;
	node->ref_id = get_next_ref_id();
	node->next = NULL;
	return node;	
}
//adds a mapping to the list of mappings
pyobj_ref_node *add_pyobj_ref_list(PyObject *pynode)
{	
	pyobj_ref_node *node = make_pyobj_ref_node(pynode);
	if(pyobj_ref_list == NULL)
	{
		pyobj_ref_list = node;
	}
	else
	{
		node->next = pyobj_ref_list;
		pyobj_ref_list = node;
	}
	return node;
}
//given a reference id, returns the corrosponding python object
PyObject *get_pyobj_ref_list(size_t ref_id)
{
	pyobj_ref_node *current = pyobj_ref_list;
	while(current!=NULL)
	{
		if(current->ref_id == ref_id)
		{
			return current->python_obj;
		}
		current = current->next;
	}
	return NULL;
}

enum prolog_term_type 
{
	INT = 0,
	FLOAT = 1,
	STRING = 2,
	LIST = 3,
	NIL = 4,
	FUNCTOR = 5,
	VAR = 6,
	REF = 7
};
int is_reference(prolog_term term)
{
	char *result = p2c_string(term);
	if(strncmp("ref_", result, 4)== 0 ){
		return 1;
	}
	return 0;
}
int find_prolog_term_type(prolog_term term)
{
	if(is_float(term))
		return FLOAT;
	else if(is_int(term))
		return INT;
	else if(is_string(term)){
		
		if(is_reference(term)){
			return REF;
		}	
		return STRING;
	}
	else if(is_list(term))
		return LIST;
	else if(is_var(term))
		return VAR;
	else if(is_nil(term))
		return NIL;
	else if(is_functor(term))
		return FUNCTOR;
	else 
		return -1;
}

int find_length_prolog_list(prolog_term V)
{
	prolog_term temp = V;
	int count= 0;
	while(!(is_nil(temp)))
	{
		p2p_car(temp);
		count++;
		temp = p2p_cdr(temp);
	}
	return count;
}

int return_to_prolog(PyObject *pValue)
{
	if(pValue == Py_None){
		return 1;
	}
	if(PyInt_Check(pValue))
	{
		int result = PyInt_AS_LONG(pValue);
		extern_ctop_int(3, result);
		return 1;
	}
	else if(PyFloat_Check(pValue))
	{
		float result = PyFloat_AS_DOUBLE(pValue);
		extern_ctop_float(3, result);
		return 1;
	}else if(PyString_Check(pValue))
	{
		char *result = PyString_AS_STRING(pValue);
		extern_ctop_string(3, result);
		return 1;
	}else if(PyList_Check(pValue))
	{
		size_t size = PyList_Size(pValue);
		size_t i = 0;
		prolog_term head, tail;
		prolog_term P = p2p_new();
		tail = P;
		
		for(i = 0; i < size; i++)
		{
			c2p_list(CTXTc tail);
			head = p2p_car(CTXTc tail);
			int temp = PyInt_AS_LONG(PyList_GetItem(pValue, i));
			c2p_int(temp, head);
			tail = p2p_cdr(tail);
		}
		c2p_nil(CTXTc tail);
		p2p_unify(P, reg_term(CTXTc 3));
		return 1;
	}else
	{
		//returns an object refernce to prolog side.
		pyobj_ref_node *node = 	add_pyobj_ref_list(pValue);
		char str[30];
		sprintf(str, "ref_%lu", node->ref_id);
		extern_ctop_string(3,str);
		return 1;
	}
	return 0;
}
void prlist2pyList(prolog_term V, PyObject *pList, int count)
{
	prolog_term temp = V;
	prolog_term head;
	PyObject *pValue;
	int i;
	for(i = 0; i <count;i++)
	{ //fill ;+i){
		head = p2p_car(temp);
		int ctemp = p2c_int(head);//currently only a list of integers, need to expand it for all types. 
//		printf("%d\n", ctemp);
		pValue = PyInt_FromLong(ctemp);
		PyList_SetItem(pList, i, pValue);
		temp = p2p_cdr(temp);
	}	
}
int set_python_argument(prolog_term temp, PyObject *pArgs,int i)
{
	PyObject *pValue;
	if(find_prolog_term_type(temp) == INT)
	{
		prolog_term argument = temp;
		int argument_int = p2c_int(argument);
		pValue = PyInt_FromLong(argument_int);
		PyTuple_SetItem(pArgs, i-1, pValue);
	}else if(find_prolog_term_type(temp) == STRING)
	{
		prolog_term argument = temp;
		char *argument_char = p2c_string(argument);
		pValue = PyString_FromString(argument_char);
		PyTuple_SetItem(pArgs, i-1, pValue);
	}else if(find_prolog_term_type(temp) == FLOAT)
	{
		prolog_term argument = temp;
		float argument_float = p2c_float(argument);
		pValue = PyFloat_FromDouble(argument_float);
		PyTuple_SetItem(pArgs, i-1, pValue);
	}
	else if(find_prolog_term_type(temp) == LIST)
	{

		prolog_term argument = temp;
		if(find_prolog_term_type(argument) == LIST)
		{
			int count = find_length_prolog_list(argument);
			//									printf("check %d", count);
			PyObject *pList = PyList_New(count);
			prlist2pyList(argument, pList, count);
			PyTuple_SetItem(pArgs, i-1, pList);
		}
	}
	else if(find_prolog_term_type(temp) == REF)
	{       //when a reference is passed.
		prolog_term argument = temp;
		char *argument_ref = p2c_string(argument);
		if(strncmp("ref_", argument_ref, 4)== 0 )
		{	//gets the reference id from the string and finds it in the list
			argument_ref = argument_ref + 4;
			size_t ref = strtoul(argument_ref, NULL, 0);
			PyObject *obj = get_pyobj_ref_list(ref);
			if(obj == NULL)
				return FALSE;
			PyTuple_SetItem(pArgs, i-1, obj);
		}
	}
	return TRUE;
}
//todo: need to refactor this code.
int callpy(CTXTdecl)
{
	setenv("PYTHONPATH", ".", 1);
	PyObject *pName = NULL, *pModule = NULL, *pFunc = NULL;
	PyObject *pArgs = NULL, *pValue = NULL;
	//PyObject *pArgs, *pValue;
	prolog_term V, temp;
	Py_Initialize();
	char *module = ptoc_string(CTXTdeclc 1);
	//char *function = ptoc_string(CTXTdeclc 2);
	pName = PyString_FromString(module);
	pModule = PyImport_Import(pName);
	if(pModule == NULL)
	{
		return FALSE;	
	}
	Py_DECREF(pName);
	//	printf("%s %s", module, function);
	V = extern_reg_term(2);
	char *function = p2c_functor(V);
	//	printf("%d",find_prolog_term_type(V));
	if(is_functor(V))
	{
		int args_count  = p2c_arity(V);

		//		printf("it recognizes function %d", args_count);	
		pFunc = PyObject_GetAttrString(pModule, function);
		Py_DECREF(pModule);
		if(pFunc && PyCallable_Check(pFunc))
		{
			pArgs = PyTuple_New(args_count);
			int i;
			for(i = 1; i <= args_count; i++)
			{
				temp = p2p_arg(V, i);
				printf("%d", i);
				if(!(set_python_argument(temp, pArgs, i)))
				{
					return FALSE;
				}
			}

		}
		pValue = PyObject_CallObject(pFunc, pArgs);
		if(return_to_prolog(pValue))
			return TRUE;
		else
			return FALSE;
	}
	else
	{
		printf("not a functor");
		return FALSE;
	}
	return TRUE;
}
