//author: Muthukumar Suresh muthukumar5393@gmail.com
#include<math.h>
#include<stdio.h>
#include<string.h>
//#include<alloca.h>
#include<python2.7/Python.h>
#include<cinterf.h>
size_t ref_id_counter = 20; //the reference id that is passed to the prolog side
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

//region : function decleration(s)
int convert_pyObj_prObj(PyObject *, prolog_term *);
int convert_prObj_pyObj(prolog_term , PyObject **);
//endregion

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
		//printf("found string");
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
			PyObject *pyObj = PyList_GetItem(pValue, i);
			int temp = PyInt_AS_LONG(pyObj);
			c2p_int(temp, head);
			//convert_pyObj_prObj(pyObj, &head);				
			tail = p2p_cdr(tail);
		}
		c2p_nil(CTXTc tail);
		p2p_unify(P, reg_term(CTXTc 3));
		return 1;
	}else
	{
		//returns an object refernce to prolog side.
		pyobj_ref_node *node = 	add_pyobj_ref_list(pValue);
		//printf("node : %p", node);	
	char str[30];
		sprintf(str, "%p", node);
		//extern_ctop_string(3,str);
	  prolog_term ref = p2p_new();
		c2p_functor("pyObject", 1, ref);
		prolog_term ref_inner = p2p_arg(ref, 1);
    c2p_string(str, ref_inner);		
		p2p_unify(ref, reg_term(CTXTc 3));	
		return 1;
	}
	return 0;
}
int prlist2pyList(prolog_term V, PyObject *pList, int count)
{
	prolog_term temp = V;
	prolog_term head;
	// PyObject *pValue;
	int i;
	for(i = 0; i <count;i++)
	{ //fill ;+i){
		head = p2p_car(temp);
		// int ctemp = p2c_int(head);//currently only a list of integers, need to expand it for all types. 
		// pValue = PyInt_FromLong(ctemp);
		PyObject *pyObj = NULL;
		if( !convert_prObj_pyObj(head, &pyObj))
			return FALSE;
		PyList_SetItem(pList, i, pyObj);
		temp = p2p_cdr(temp);
	}	
	return TRUE;
}

int convert_pyObj_prObj(PyObject *pyObj, prolog_term *prTerm)
{
	if(pyObj == Py_None){
		return 1;// todo: check this case for a list with a none in the list. how does prolog side react 
	}
	if(PyInt_Check(pyObj))
	{
		int result = PyInt_AS_LONG(pyObj);
		//extern_ctop_int(3, result);
		c2p_int(result, *prTerm);
		return 1;
	}
	else if(PyFloat_Check(pyObj))
	{
		float result = PyFloat_AS_DOUBLE(pyObj);
		//extern_ctop_float(3, result);
		c2p_float(result, *prTerm);
		return 1;
	}else if(PyString_Check(pyObj))
	{
		char *result = PyString_AS_STRING(pyObj);
		//extern_ctop_string(3, result);
		c2p_string(result, *prTerm);
		return 1;
	}else if(PyList_Check(pyObj))
	{
		size_t size = PyList_Size(pyObj);
		size_t i = 0;
		prolog_term head, tail;
		prolog_term P = p2p_new();
		tail = P;
		
		for(i = 0; i < size; i++)
		{
			c2p_list(CTXTc tail);
			head = p2p_car(CTXTc tail);
			PyObject *pyObjInner = PyList_GetItem(pyObj, i);
			//int temp = PyInt_AS_LONG(pyObj);
			//c2p_int(temp, head);
			convert_pyObj_prObj(pyObjInner, &head);				
			tail = p2p_cdr(tail);
		}
		c2p_nil(CTXTc tail);
		//p2p_unify(P, reg_term(CTXTc 3));
		return 1;
	}else
	{
		//returns an object refernce to prolog side.
		pyobj_ref_node *node = 	add_pyobj_ref_list(pyObj);
		//printf("node : %p", node);	
		char str[30];
		sprintf(str, "%p", node);
		//extern_ctop_string(3,str);
	  	prolog_term ref = p2p_new();
		c2p_functor("pyObject", 1, ref);
		prolog_term ref_inner = p2p_arg(ref, 1);
    	c2p_string(str, ref_inner);		
		p2p_unify(ref, *prTerm);	
		return 1;
	}
}
int convert_prObj_pyObj(prolog_term prTerm, PyObject **pyObj)
{
	if(find_prolog_term_type(prTerm) == INT)
	{
		prolog_term argument = prTerm;
		int argument_int = p2c_int(argument);
		*pyObj = PyInt_FromLong(argument_int);
		return TRUE;
	}else if(find_prolog_term_type(prTerm) == STRING)
	{
		prolog_term argument = prTerm;
		char *argument_char = p2c_string(argument);
		*pyObj = PyString_FromString(argument_char);
		return TRUE;
	}else if(find_prolog_term_type(prTerm) == FLOAT)
	{
		prolog_term argument = prTerm;
		float argument_float = p2c_float(argument);
		*pyObj = PyFloat_FromDouble(argument_float);
		return TRUE;
	}
	else if(find_prolog_term_type(prTerm) == LIST)
	{

		prolog_term argument = prTerm;
		if(find_prolog_term_type(argument) == LIST)
		{
			int count = find_length_prolog_list(argument);
			PyObject *pList = PyList_New(count);
			if(!prlist2pyList(argument, pList, count))
				return FALSE;
			*pyObj = pList;
			return TRUE;
		}
	}
	// else if(find_prolog_term_type(prTerm) == REF)
	// {       //when a reference is passed.
	// 	prolog_term argument = prTerm;
	// 	char *argument_ref = p2c_string(argument);
	// 	if(strncmp("ref_", argument_ref, 4)== 0 )
	// 	{	//gets the reference id from the string and finds it in the list
	// 		argument_ref = argument_ref + 4;
	// 		size_t ref = strtoul(argument_ref, NULL, 0);
	// 		PyObject *obj = get_pyobj_ref_list(ref);
	// 		if(obj == NULL)
	// 			return FALSE;
	// 		*pyObj = obj;
	// 		return TRUE;
	// 	}
	// }
	else if (find_prolog_term_type(prTerm) == FUNCTOR)
	{
		//region begin : handle pyobject term
		if(strcmp(p2c_functor(prTerm),"pyObject") == 0)
		{
			prolog_term ref = p2p_arg(prTerm, 1);
			char *node_pointer = p2c_string(ref); 
			pyobj_ref_node *pyobj_ref = (pyobj_ref_node *)strtol(node_pointer,NULL, 0);  
			//pyobj_ref_node *node = (pyobj_ref_node *)(node_pointer); 
			*pyObj = pyobj_ref->python_obj;

		}
		//end region

	}
	return FALSE;
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
		//printf("%s", argument_char);
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
		
		int count = find_length_prolog_list(argument);
		PyObject *pList = PyList_New(count);
		if (! prlist2pyList(argument, pList, count))
			return FALSE;
		PyTuple_SetItem(pArgs, i-1, pList);
		
	}
	else if (find_prolog_term_type(temp) == FUNCTOR)
	{
		//region begin : handle pyobject term
		if(strcmp(p2c_functor(temp),"pyObject") == 0)
		{
			prolog_term ref = p2p_arg(temp, 1);
			char *node_pointer = p2c_string(ref); 
			pyobj_ref_node *pyobj_ref = (pyobj_ref_node *)strtol(node_pointer,NULL, 0);  
			//pyobj_ref_node *node = (pyobj_ref_node *)(node_pointer); 
			pValue = pyobj_ref->python_obj;

			//printf("set_argN: %p %zu" , pValue, pyobj_ref->ref_id);
			PyTuple_SetItem(pArgs, i - 1 , pValue);
		}
		//end region

	}
	// else if(find_prolog_term_type(temp) == REF)
	// {       //when a reference is passed.
	// 	prolog_term argument = temp;
	// 	char *argument_ref = p2c_string(argument);
	// 	if(strncmp("ref_", argument_ref, 4)== 0 )
	// 	{	//gets the reference id from the string and finds it in the list
	// 		argument_ref = argument_ref + 4;
	// 		size_t ref = strtoul(argument_ref, NULL, 0);
	// 		PyObject *obj = get_pyobj_ref_list(ref);
	// 		if(obj == NULL)
	// 			return FALSE;
	// 		PyTuple_SetItem(pArgs, i-1, obj);
	// 	}
	// }

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
	V = extern_reg_term(2);
	char *function = p2c_functor(V);
	if(is_functor(V))
	{
		int args_count = p2c_arity(V);

		pFunc = PyObject_GetAttrString(pModule, function);
		Py_DECREF(pModule);
		if(pFunc && PyCallable_Check(pFunc))
		{
			pArgs = PyTuple_New(args_count);
			int i;
			for(i = 1; i <= args_count; i++)
			{
				temp = p2p_arg(V, i);
				if(!(set_python_argument(temp, pArgs, i)))
				{
					return FALSE;
				}
			}

		}
		else
		{
			return FALSE;
		}
		
		pValue = PyObject_CallObject(pFunc, pArgs);
		//printf("return call : %p\n",pValue); 
		if(return_to_prolog(pValue))
			return TRUE;
		else
			return FALSE;
	}
	else
	{
		return FALSE;
	}
	return TRUE;
}
