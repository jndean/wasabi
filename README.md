# WASABI

A jokebox CPython extension, liable to leave a bad taste in your mouth. Not recommended for production code. Or at all.

### Mutating immutable variables
Wasabi provides methods for mutating some of Python's immutable types, to hilarious effect.
Currently only ints, floats, tuples and bytes are implemented. Remember that integers from -5 to 256 and bytes objects of length 1 are stored as singletons, which makes modifying them extra dangerous.

```python
>>> x = 50
>>> wasabi.set_int(50, 53)
>>> print(x)  # 53
>>> x = 50
>>> print(x)  # 53
>>> print(50)  # 53
>>> print(25 + 25)  # 53
>>> print(50 == 53)  # True
>>> print(50 < 52)  # False

>>> wasabi.set_int(90, 0)
>>> x = 95
>>> while x:
...     print(x)
...     x -= 1
# 95, 94, 93, 92, 91
```

```python
>>> def func(B, T, F, I):
...     wasabi.set_bytes(B, b'x' * len(B))
...     for i in range(len(T)):
...         wasabi.set_tuple_item(T, i, 'wasabi')
...     wasabi.set_float(F, 1.701)
...     wasabi.set_int(I, 1701)

>>> B = b'My bytestring'
>>> T = (1, 2, 3, 4)
>>> F = 0.0
>>> I = 999999

>>> func(B, T, F, I)  # Surely my immutables will be safe?

>>> print(B)  # b'xxxxxxxxxxxxx'
>>> print(T)  # ('wasabi', 'wasabi', 'wasabi', 'wasabi')
>>> print(F)  # 1.701
>>> print(I)  # 1701
```

### Monkey patching built-ins
Wasabi provides a method for monkey patching attributes onto Python's built-in types, which is ordinarily not possible. This actually has real-world applications, but for those you should probably use the much more portable (and sensibly restrictive) [forbiddenfruit](https://github.com/clarete/forbiddenfruit) module.

```python
>>> wasabi.set_attr(int, "new_property", 100)
>>> x = 5
>>> print(x.new_property)  # 100
>>> print(int.new_property)  # 100

>>> wasabi.set_attr(bytes, "backwards", lambda x: x[::-1])
>>> x = b'wasabi'
>>> print(x.backwards())  # b'ibasaw'

>>> wasabi.set_attr(type(...), int, "Gross")
```

### Recovering free'd floats
For some reason Wasabi provides a way to recover floats that have been recently garbage collected. This has no applications.

```python
>>> def important_function(secret_input):
...     secret_floats = [random.random() for i in range(10)]
...     return secret_input + secret_floats[-1]

>>> public_output = important_function(secret_input=99.9)

>>> recent_floats = wasabi.get_recent_floats()
>>> deduced_secret_input = public_output - recent_floats[-1]
>>> print(deduced_secret_input)  # 99.9
```

Due to the implemention needing to decref a float to get a pointer to the free floats list, the very most recent float is trampled and hence not recoverable.

## Comments
Tested on Python 3.5.2 only, though I expect it to do okay on at least newer versions as well.

Misuse (use) of this module is a good way to crash you python interpreter. For example, the integer 1 is used a lot internally, and changing its value is basically guarenteed to segfault the next function call.

Immutable items are immutable for a reason, and the interpreter expects them to remain unmutated. As such, the behaviour of this module is likely to vary wildly depending on the context of execution, even when it doesn't segfault. In particular, watch out for how 'constant' items may be treated by the python byte-code compiler.


Mutating a str is not currently supported because unicode is a headache. Mutating a bool is not supported since CPython doesn't actually care about the contents of True and False (though it turns out they do contain a 1 and 0). Since they're singletons it is enough to only ever compare pointers to bools.