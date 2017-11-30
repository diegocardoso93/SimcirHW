
require "../circuitlib"
local test = require "../pl/test"
local asserteq = test.asserteq

asserteq(bitnot(0),     1)
asserteq(0 -bitor- 1,   1)
asserteq(1 -bitand- 1,  1)
asserteq(1 -bitxor- 0,  1)
asserteq(0 -bitnand- 0, 1)

asserteq(bitnot(1),     0)
asserteq(0 -bitor- 0,   0)
asserteq(1 -bitand- 0,  0)
asserteq(1 -bitxor- 1,  0)
asserteq(1 -bitnand- 1, 0)

ckt = convert_circuit("(A OR B) AND NOT(B XOR NOT(A))")
asserteq(ckt, '(A -bitor- B) -bitand- bitnot(B -bitxor- bitnot(A))')

A = 0; B = 1
asserteq((A -bitor- B) -bitand- bitnot((B -bitxor- bitnot(A))), 1)
