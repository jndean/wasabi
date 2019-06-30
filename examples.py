#!/usr/bin/python3

import wasabi
import random


print('\n-------------------- Singleton Integers (-5 to 256) --------------------')
x = 50

wasabi.set_int(50, 53)

print(x)  # 53
x = 50
print(x)  # 53
print(50)  # 53
print(25 + 25)  # 53
print(53 - 3)  # 53
print(bin(50))  # 0b110101
print(50 == 53)  # True
print(50 < 52)  # False

wasabi.set_int(90, 0)
x = 95
while x:
    print('-', x)
    x -= 1
#- 95
#- 94
#- 93
#- 92
#- 91

# We have to provide a method to reset these small singleton ints, since
# wasabi.set_int can't be used when you no longer have a way to convey the
# original value to the python vmachine.
# I.e, typing wasabi.set_int(50, 50) clearly won't do what you want.
wasabi.reset_small_int(50)
wasabi.reset_small_int(90)
print(50)  # 50


print('\n------------ Bytes, Tuples, Floats, Non-singleton Integers ------------')

def func(B, T, F, I):
    wasabi.set_bytes(B, b'x' * len(B))
    for i in range(len(T)):
        wasabi.set_tuple_item(T, i, 'wasabi')
    wasabi.set_float(F, 1.701)
    wasabi.set_int(I, 1701)

B = b'My bytestring'
T = (1, 2, 3, 4)
F = 0.0
I = 999999

# Surely my immutables will be safe?
func(B, T, F, I)

# !
print(B)  # b'xxxxxxxxxxxxx'
print(T)  # ('wasabi', 'wasabi', 'wasabi', 'wasabi')
print(F)  # 1.701
print(I)  # 1701

# Also, remember that byte strings of length 1 are singletons,
# and so behave like the small Singleton Integers in the example above
    

print('\n--------------------- Monkey Patching --------------------')


wasabi.set_attr(int, "new_property", 100)
x = 5
print(x.new_property)  # 100
print(int.new_property)  # 100

wasabi.set_attr(bytes, "backwards", lambda x: x[::-1])
x = b'wasabi'
print(x.backwards())  # b'ibasaw'

wasabi.set_attr(type(...), "...", "Gross")
x = type(...)
print(getattr(x, "..."))  # "Gross"


print('\n------------------ Recently used floats ------------------')

def important_function(secret_input):
    secret_floats = [random.random() for i in range(10)]
    return secret_input + secret_floats[-1]

public_output = important_function(secret_input=99.9)

recent_floats = wasabi.get_recent_floats()
deduced_secret_input = public_output - recent_floats[-1]
print(deduced_secret_input)  # 99.9
