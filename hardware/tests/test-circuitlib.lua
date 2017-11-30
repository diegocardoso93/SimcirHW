
require "../circuitlib"

print(0 -bitor- 1)
print(1 -bitand- 1)
print(1 -bitxor- 0)
print(1 -bitor- 0)
print(0 -bitnand- 0)

print(convert_circuit("(A AND B) OR (NOT(B) XOR A)"))