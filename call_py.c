//author: Muthukumar Suresh muthukumar5393@gmail.com
#include<math.h>
#include<stdio.h>
#include<string.h>
//#include<alloca.h>
#include<python2.7/Python.h>
#include<cinterf.h>

enum prolog_term_type {
		INT = 0,
		FLOAT = 1,
		STRING = 2,
		LIST = 3,
		NIL = 4,
		FUNCTOR = 5,
		VAR = 6
		
};

int find_prolog_term_type(prolog_term term){
	if(is_float(term))
		return FLOAT;
	else if(is_int(term))
		return INT;
	else if(is_string(term))
		return STRING;
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

int find_length_prolog_list(prolog_term V){
	prolog_term temp = V;
	int count= 0;
	while(!(is_nil(temp))){
		p2p_car(temp);
		count++;
		temp = p2p_cdr(temp);
	}
	return count;
}

int return_to_prolog(PyObject *pValue){
	if(PyInt_Check(pValue)){
		int result = PyInt_AS_LONG(pValue);
		extern_ctop_int(3, result);
		return 1;
	}
	else if(PyFloat_Check(pValue)){
		float result = PyFloat_AS_DOUBLE(pValue);
		extern_ctop_float(3, result);
		return 1;
	}else if(PyString_Check(pValue)){
		char *result = PyString_AS_STRING(pValue);
		extern_ctop_string(3, result);
		return 1;
	}else if(PyList_Check(pValue)){
		size_t size = PyList_Size(pValue);
		size_t i = 0;
		prolog_term head, tail;
		prolog_term P = p2p_new();
		tail = P;
		
		for(i = 0; i < size; i++){
			c2p_list(CTXTc tail);
			head = p2p_car(CTXTc tail);
			int temp = PyInt_AS_LONG(PyList_GetItem(pValue, i));
			c2p_int(temp, head);
			tail = p2p_cdr(tail);
		}
		c2p_nil(CTXTc tail);
		p2p_unify(P, reg_term(CTXTc 3));
		return 1;
	}
	return 0;
}
void prlist2pyList(prolog_term V, PyObject *pList, int count){
	prolog_term temp = V;
	prolog_term head;
	PyObject *pValue;
	int i;
	for(i = 0; i <count;i++){ //fill ;+i){
		head = p2p_car(temp);
		int ctemp = p2c_int(head);//currently only a list of integers, need to expand it for all types. 
//		printf("%d\n", ctemp);
		pValue = PyInt_FromLong(ctemp);
		PyList_SetItem(pList, i, pValue);
		temp = p2p_cdr(temp);
	}	
}

//todo: need to refactor this code.
int callpy(CTXTdecl){
	setenv("PYTHONPATH", ".", 1);
	PyObject *pName, *pModule, *pFunc;
	PyObject *pArgs, *pValue;
	//PyObject *pArgs, *pValue;
	prolog_term V, temp;
	Py_Initialize();
	char *module = ptoc_string(CTXTdeclc 1);
	//char *function = ptoc_string(CTXTdeclc 2);
	pName = PyString_FromString(module);
	pModule = PyImport_Import(pName);
	Py_DECREF(pName);
//	printf("%s %s", module, function);
	V = extern_reg_term(2);
	char *function = p2c_functor(V);
//	printf("%d",find_prolog_term_type(V));
	if(is_functor(V)){
		int args_count  = p2c_arity(V);
		
//		printf("it recognizes function %d", args_count);	
		pFunc = PyObject_GetAttrString(pModule, function);
		if(pFunc && PyCallable_Check(pFunc)){
			pArgs = PyTuple_New(args_count);
			int i;
			for(i = 1; i <= args_count; i++){
				temp = p2p_arg(V, i);
				if(find_prolog_term_type(temp) != FUNCTOR){
					printf("error: illegal type for argument");
					return FALSE;
				}
				else{
					int arg_len = p2c_arity(temp);
					if(arg_len != 2){
						printf("error: argument length wrong");
						return FALSE;
					}
					else{
						prolog_term arg_type = p2p_arg(temp, 2);
						int type = find_prolog_term_type(arg_type);
						if(find_prolog_term_type(arg_type) != STRING){
							printf("Error: argument type wrong %d", type);
							return FALSE;
						}
						else{
							char *type = p2c_string(arg_type);
							if(strcmp(type, "int") == 0){
								prolog_term argument = p2p_arg(temp, 1);
								int argument_int = p2c_int(argument);
								pValue = PyInt_FromLong(argument_int);
								PyTuple_SetItem(pArgs, i-1, pValue);
							}else if(strcmp(type, "string") == 0){
								prolog_term argument = p2p_arg(temp, 1);
								char *argument_char = p2c_string(argument);
								pValue = PyString_FromString(argument_char);
								PyTuple_SetItem(pArgs, i-1, pValue);
							}else if(strcmp(type, "float") == 0){
								prolog_term argument = p2p_arg(temp, 1);
								float argument_float = p2c_float(argument);
								pValue = PyFloat_FromDouble(argument_float);
								PyTuple_SetItem(pArgs, i-1, pValue);
							}
							else if(strcmp(type, "list") == 0){
								
								prolog_term argument = p2p_arg(temp, 1);
								if(find_prolog_term_type(argument) == LIST){
									int count = find_length_prolog_list(argument);
//									printf("check %d", count);
									PyObject *pList = PyList_New(count);
									prlist2pyList(argument, pList, count);
									PyTuple_SetItem(pArgs, i-1, pList);
								}
							}
						}
					}
				}
			}
			pValue = PyObject_CallObject(pFunc, pArgs);
			if(return_to_prolog(pValue))
				return TRUE;
			else
				return FALSE;
		}
	}else{
		printf("not a functor");
		return FALSE;
	}
	return TRUE;
}
