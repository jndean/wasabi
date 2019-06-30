#include<stdio.h>
#include<stddef.h>
#include<stdlib.h>

#include<Python.h>



// ------------------------------ Integers ------------------------------ //
static PyLongObject* small_ints = NULL;
static const int nsmallposints = 257;
static const int nsmallnegints = 5;


PyObject* wasabi_SetInt(PyObject* self, PyObject* args){

  PyLongObject *target = NULL, *source = NULL;
  if(!PyArg_ParseTuple(args, "OO", &target, &source))
    return NULL;

  if(!PyLong_Check(target) || !PyLong_Check(source)){
    PyErr_SetString(PyExc_ValueError, "set_int requires two integers.");
    return NULL;
  }

  /* Fail on insufficient space */
  Py_ssize_t target_size = abs(Py_SIZE(target));
  Py_ssize_t source_size = abs(Py_SIZE(source));
  if(target_size <source_size) {
    PyErr_SetString(PyExc_ValueError, "Target value too small to store payload.");
    return NULL;
  }

  /* Copy across the absolute value */
  memcpy(target->ob_digit, source->ob_digit, source_size * sizeof(digit));
  memset(target->ob_digit + source_size * sizeof(digit), 0, target_size - source_size);

  /* Copy across the sign */
  int target_sgn =  Py_SIZE(target) == 0 ? 0 : (Py_SIZE(target) < 0 ? -1 : 1);
  int source_sgn =  Py_SIZE(source) == 0 ? 0 : (Py_SIZE(source) < 0 ? -1 : 1);
  Py_SIZE(target) *= target_sgn * source_sgn;      
  
  Py_RETURN_NONE;
}


PyObject* wasabi_ResetSmallInt(PyObject* self, PyObject* args){

  PyLongObject *target = NULL;
  if(!PyArg_ParseTuple(args, "O", &target))
    return NULL;
  if(!PyLong_Check(target)){
    PyErr_SetString(PyExc_ValueError, "reset_small_int requires an integer.");
    return NULL;
  }

  /* Find beginning of small_ints table */
  if(NULL == small_ints){
    PyLongObject* small_ints_tmp = (PyLongObject*)PyLong_FromLong(-5);
    if(NULL == small_ints_tmp) return NULL;
    small_ints = small_ints_tmp;
    Py_DECREF(small_ints_tmp);
  }

  /* Fail if given an int not in the small_int table */
  if(target < small_ints || target >= small_ints + nsmallposints + nsmallnegints){
     PyErr_SetString(PyExc_ValueError, "Specified int is not a singleton.");
    return NULL;
  }
  
  // Set the value */
  *target->ob_digit = abs((target - small_ints) - nsmallnegints);
  Py_SIZE(target) *= 1 - (((target < small_ints + nsmallnegints) ^
			   (Py_SIZE(target) < 0)) << 1);
  
  Py_RETURN_NONE;
}


// --------------------------------- Bytes --------------------------------- //

PyObject* wasabi_SetBytes(PyObject* self, PyObject* args){

  Py_buffer target_view = {NULL}, source_view = {NULL};
  if(!PyArg_ParseTuple(args, "y*y*", &target_view, &source_view))
    return NULL;

  unsigned char* target = target_view.buf;
  Py_ssize_t target_len = target_view.len;
  unsigned char* source = source_view.buf;
  Py_ssize_t source_len = source_view.len;

  memcpy(target, source, Py_MIN(target_len, source_len));
  
  PyBuffer_Release(&target_view);
  PyBuffer_Release(&source_view);
  Py_RETURN_NONE;
}

PyObject* wasabi_SetSingleBytes(PyObject* self, PyObject* args){

  Py_buffer target_view = {NULL};
  unsigned char value;
  if(!PyArg_ParseTuple(args, "y*b", &target_view, &value))
    return NULL;

  unsigned char* target = target_view.buf;
  Py_ssize_t target_len = target_view.len;
  if(target_len != 1){
    PyErr_SetString(PyExc_ValueError,
		    "set_single_bytes requires a bytes object of length 1.");
    return NULL;
  }

  *target = value;
  
  PyBuffer_Release(&target_view);
  Py_RETURN_NONE;
}

// ---------------------------------- Tuples ---------------------------------- //

PyObject* wasabi_SetTupleItem(PyObject* self, PyObject* args){

  PyObject *op = NULL, *newitem = NULL;
  Py_ssize_t i = -1;
  
  if(!PyArg_ParseTuple(args, "OnO:set_tuple_item", &op, &i, &newitem))
    return NULL;

  if (!PyTuple_Check(op)) {
    PyErr_BadInternalCall();
    return NULL;
  }
  if (i < 0 || i >= Py_SIZE(op)) {
    PyErr_SetString(PyExc_IndexError, "tuple assignment index out of range");
    return NULL;
  }
  
  Py_INCREF(newitem);
  PyObject **p = ((PyTupleObject *)op) -> ob_item + i;
  Py_XSETREF(*p, newitem);

  Py_RETURN_NONE;
}



// ---------------------------------- Floats ----------------------------------- //

PyObject* wasabi_SetFloat(PyObject* self, PyObject* args){

  PyObject *target = NULL, *payload = NULL;
  
  if(!PyArg_ParseTuple(args, "OO:set_float", &target, &payload))
    return NULL;

  if (!PyFloat_Check(target) || !PyFloat_Check(payload)){
    PyErr_SetString(PyExc_ValueError, "Both arguents should be floats");
    return NULL;
  }

  ((PyFloatObject*) target)->ob_fval = ((PyFloatObject*) payload)->ob_fval;

  Py_RETURN_NONE;
}


PyObject* wasabi_GetFreeFloats(PyObject* self, PyObject* args){
  long num_returned = 0;
  double* vals = NULL;
  PyObject* output = NULL;
  
  unsigned int max_ints = -1;
  if(!PyArg_ParseTuple(args, "|I:get_free_floats", &max_ints))
    goto failure;

  // To get a pointer to the free_list, create a float and free it //
  // This will consume one of the free floats //
  PyObject* free_list = PyFloat_FromDouble(0);
  Py_DECREF(free_list);
    
  // The free_list is a linked list, where the next free float is pointed at
  // using the ob_type field of the current float struct
  // The first item will be the 0.0 we freed above, which isn't interesting, so
  // we skip it.
  PyObject* next_float = free_list;
  while(NULL != Py_TYPE(next_float)) {
    next_float = (PyObject*) Py_TYPE(next_float);
    num_returned++;
  }

  // Read off values without damaging them by allocating new PyFloats //
  vals = malloc(num_returned * sizeof(double));
  next_float = free_list;
  for (int i = 0; i < num_returned; i++) {
    next_float = (PyObject*) Py_TYPE(next_float);
    vals[i] = ((PyFloatObject*) next_float)->ob_fval;
  }

  // Create the output //
  output = PyList_New(num_returned);
  if (!output) goto failure;
  for (int i = 0; i < num_returned; i++) {
    PyObject* f = PyFloat_FromDouble(vals[i]);
    if (!f) goto failure;
    PyList_SET_ITEM(output, i, f);
  }

  if (vals) free(vals);
  return output;
  
 failure:
  if (vals) free(vals);
  if (output) {
    for (int i = 0; i < num_returned; i++)
      Py_XDECREF(PyList_GET_ITEM(output, i));
    Py_DECREF(output);
  }
  return NULL;
}

// ----------------------------- Strings (Unicode) ----------------------------- //

/*PyObject* wasabi_SetString(PyObject* self, PyObject* args){

  PyObject *target = NULL, *payload = NULL;
  
  if(!PyArg_ParseTuple(args, "OO", &target, &payload))
    return NULL;
  
  printf("%d\n", PyUnicode_Check(target));
  (((PyASCIIObject*)(op))->wstr)(((PyASCIIObject*)(op))->wstr)(((PyASCIIObject*)(op))->wstr)
  Py_RETURN_TRUE;
  }*/


// ------------------------------ Monkey patching ------------------------------//


// Recreate this here because it's not visible through Python.h //
typedef struct {
    PyObject_HEAD
    PyObject *mapping;
} mappingproxyobject;


PyObject* wasabi_MonkeyPatch(PyObject* self, PyObject* args){

  PyObject *target = NULL, *name = NULL, *payload = NULL;
  PyObject *dict_proxy = NULL, *dict = NULL, *existing = NULL;
  
  if(!PyArg_ParseTuple(args, "OOO", &target, &name, &payload))
    goto error_cleanup;
  
  if (NULL == (dict_proxy = PyObject_GetAttrString(target, "__dict__")))
    goto error_cleanup;

  // __dict__ could actually be a mappingproxy object. If so, unwrap that. //
  if (PyDict_Check(dict_proxy)) dict = dict_proxy;
  else dict = ((mappingproxyobject*)dict_proxy)->mapping;

  if(NULL != (existing = PyDict_GetItem(dict, name))){
    if(PyErr_Occurred()) goto error_cleanup;    
    if(Py_TYPE(existing) == &PyWrapperDescr_Type){
      //wrapperfunc wrapper = ((PyWrapperDescrObject *)existing)->d_base->wrapper;
      // MIDLERTIDIG: NEED TO CHECK IF THE ATTRIBUTE IS A SLOT (WRAPPER)
      printf("That's a wrapper descr type, not touching that yet...\n");
      Py_DECREF(dict_proxy);
      Py_RETURN_NONE;
    }
  }

  if(-1 == PyDict_SetItem(dict, name, payload)){
    printf("Failed to set item\n");
    goto error_cleanup;
  }
  
  Py_DECREF(dict_proxy);
  Py_RETURN_NONE;

 error_cleanup:
  Py_XDECREF(dict_proxy);
  return NULL;
}






PyObject* (*old_long_xor)(PyObject*, PyObject*) = NULL;
PyObject* my_long_xor = NULL;
PyObject* my_long_xor_wrapper(PyObject* a, PyObject* b){
  return PyObject_CallFunctionObjArgs(my_long_xor, a, b, NULL);
}

PyObject* wasabi_test(PyObject* self, PyObject* args){

  PyObject *input = NULL, *integer;
  
  if(-1 == PyArg_ParseTuple(args, "OO", &input, &integer))
    goto error_cleanup;

  if(!PyCallable_Check(input)){
    PyErr_SetString(PyExc_ValueError, "Require a callable object");
    goto error_cleanup;
  }
  
  PyNumberMethods* int_number_methods = PyLong_Type.tp_as_number;
  if(NULL == old_long_xor) old_long_xor = int_number_methods->nb_xor;
  
  Py_INCREF(input);
  Py_XDECREF(my_long_xor);
  my_long_xor = input;
  int_number_methods->nb_xor = my_long_xor_wrapper;


  // Replacing slot //
  PyObject* dict_proxy = PyObject_GetAttrString(integer, "__dict__");
  PyObject* dict = ((mappingproxyobject*)dict_proxy)->mapping;
  
  PyObject* xor = PyDict_GetItemString(dict, "__xor__");
  if(PyErr_Occurred()) return NULL;
  if(NULL == xor) {
    PyErr_SetString(PyExc_ValueError, "No method __xor__");
    goto error_cleanup;
  }

  ((PyWrapperDescrObject*)xor)->d_wrapped = my_long_xor;
  
  Py_DECREF(dict_proxy);
  Py_DECREF(xor);
  
  Py_RETURN_NONE;
  
 error_cleanup:
  return NULL;
}




// ------------------------------ Module Setup ------------------------------ //

static PyMethodDef WasabiMethods[] = {
    {"set_int",  wasabi_SetInt, METH_VARARGS,
     "."},
    {"reset_small_int",  wasabi_ResetSmallInt, METH_VARARGS,
     "."},
    {"set_bytes",  wasabi_SetBytes, METH_VARARGS,
     "."},
    {"set_single_bytes",  wasabi_SetSingleBytes, METH_VARARGS,
     "."},
    {"set_attr",  wasabi_MonkeyPatch, METH_VARARGS,
     "."},
    {"test",  wasabi_test, METH_VARARGS,
     "."},
    {"set_tuple_item", wasabi_SetTupleItem, METH_VARARGS,
     "."},
    {"set_float", wasabi_SetFloat, METH_VARARGS,
     "."},
    {"get_recent_floats", wasabi_GetFreeFloats, METH_VARARGS,
     "."},
    //{"unwrap",  wasabi_Unwrap, METH_VARARGS,
    // "."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef wasabimodule = {
   PyModuleDef_HEAD_INIT,
   "wasabi",  
   NULL, 
   -1,
   WasabiMethods
};

PyMODINIT_FUNC PyInit_wasabi(void)
{
    return PyModule_Create(&wasabimodule);
}
