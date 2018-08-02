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
  if(target_size <source_size) Py_RETURN_FALSE;

  /* Copy across the absolute value */
  memcpy(target->ob_digit, source->ob_digit, source_size * sizeof(digit));
  memset(target->ob_digit + source_size * sizeof(digit), 0, target_size - source_size);

  /* Copy across the sign */
  int target_sgn =  Py_SIZE(target) == 0 ? 0 : (Py_SIZE(target) < 0 ? -1 : 1);
  int source_sgn =  Py_SIZE(source) == 0 ? 0 : (Py_SIZE(source) < 0 ? -1 : 1);
  Py_SIZE(target) *= target_sgn * source_sgn;      
  
  Py_RETURN_TRUE;
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
  if(target < small_ints || target >= small_ints + nsmallposints + nsmallnegints)
    Py_RETURN_FALSE;
  
  // Set the value */
  *target->ob_digit = abs((target - small_ints) - nsmallnegints);
  Py_SIZE(target) *= 1 - (((target < small_ints + nsmallnegints) ^ (Py_SIZE(target) < 0)) << 1);
  
  Py_RETURN_TRUE;
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
  Py_RETURN_TRUE;
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
  Py_RETURN_TRUE;
}


// ------------------------------ Monkey patching ------------------------------//

// Recreate this here because it's not visible through Python.h //
typedef struct {
    PyObject_HEAD
    PyObject *mapping;
} mappingproxyobject;


PyObject* wasabi_MonkeyPatch(PyObject* self, PyObject* args){

  PyObject *target = NULL, *name = NULL, *payload = NULL;
  PyObject *dict_proxy = NULL, *dict = NULL;
  
  if(!PyArg_ParseTuple(args, "OOO", &target, &name, &payload))
    goto error_cleanup;
  
  if (NULL == (dict_proxy = PyObject_GetAttrString(target, "__dict__")))
    goto error_cleanup;

  // __dict__ could actually be a mappingproxy object. If so, unwrap that. //
  if (PyDict_Check(dict_proxy)) dict = dict_proxy;
  else dict = ((mappingproxyobject*)dict_proxy)->mapping;

  // MIDLERTIDIG: NEED TO CHECK IF THE ATTRIBUTE IS A SLOT (WRAPPER)
  /*if(Py_TYPE(dict) == &PyWrapperDescr_Type){
    printf("That's a wrapper descr type, not touching that yet...\n");
    Py_DECREF(dict_proxy);
    Py_RETURN_NONE;
    }*/
  if(-1 == PyDict_SetItem(dict, name, payload))
   goto error_cleanup;
  
  Py_DECREF(dict_proxy);
  Py_RETURN_NONE;

 error_cleanup:
  Py_XDECREF(dict_proxy);
  return NULL;
}

/*PyObject* wasabi_Unwrap(PyObject* self, PyObject* args){
  PyObject* wrapper = NULL;
  if(!PyArg_ParseTuple(args, "O", &wrapper))
    return NULL;
  printf("%d\n", Py_TYPE(wrapper) == &_PyMethodWrapper_Type);
  //printf("%s\n", wrapper.tp)
  //PyObject* value = wrapper
  Py_RETURN_NONE;
  }*/

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
