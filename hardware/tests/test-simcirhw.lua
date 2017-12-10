
local SimcirHW = require '../simcirhw'

SCH = SimcirHW:new()

SCH:receive_circuit(
[===[ 
{
  outputs={
    Out1="(In1 AND NOT(In3)) OR (In2 AND In3)",
    Out2="A OR B"
  }
  ,inputs={
    In1={values={0,1,0,1},timeslices={200,200,400,600},infinity=true},
    In2="1",
    In3="0"
  }
  ,pin_map={
    Out1="D0",
    Out2="D1"
  }
  ,hardware="esp8266"
}
]===])

SCH:parse_circuit()
SCH:configure_timeslices()
SCH:start()

-- test static inputs
-- assert(SCH:read_pin("D2") == 1)


print("-- end of tests --")
