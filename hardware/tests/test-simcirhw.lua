
local SimcirHW = require '../simcirhw'
require "../circuitlib"

SCH = SimcirHW:new()

SCH:receive_circuit(
[===[ 
{
  outputs={
    Out1="(In1 AND NOT(In3)) OR (In2 AND In3)",
    Out2="In1 XOR In2"
  }
  ,inputs={
    In1=1,
    In2=0,
    In3={values={0,1,0,1},timeslices={100,200,100,600}},
  }
  -- mapped to external pins
  ,pin_map={
    Out1="D0",
    Out2="D1"
  }
  ,hardware="esp8266"
  ,cycles=1 -- 0 => infinity
}
]===])

SCH:parse_circuit()

-- test if parse simcirhw expression format correctly
assert(SCH:get_expression("Out1") == "(In1 -bitand- bitnot(In3)) -bitor- (In2 -bitand- In3)")
assert(SCH:get_expression("Out2") == "In1 -bitxor- In2")

-- test final outputs
SCH:start()
In1 = 1; In2 = 0; In3 = 1
assert(SCH.state.outputs["Out1"] == ((In1 -bitand- bitnot(In3)) -bitor- (In2 -bitand- In3)))
assert(SCH.state.outputs["Out2"] == In1 -bitxor- In2)

print("-- end of tests --")