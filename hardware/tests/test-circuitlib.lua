
require "../circuitlib"

assert(bitnot(0)     == 1)
assert(0 -bitor- 1   == 1)
assert(1 -bitand- 1  == 1)
assert(1 -bitxor- 0  == 1)
assert(0 -bitnand- 0 == 1)

assert(bitnot(1)     == 0)
assert(0 -bitor- 0   == 0)
assert(1 -bitand- 0  == 0)
assert(1 -bitxor- 1  == 0)
assert(1 -bitnand- 1 == 0)

ckt = convert_circuit("(A OR B) AND NOT(B XOR NOT(A))")
assert(ckt == '(A -bitor- B) -bitand- bitnot(B -bitxor- bitnot(A))')

A = 0; B = 1
assert((A -bitor- B) -bitand- bitnot((B -bitxor- bitnot(A))) == 1)

print("end of tests")