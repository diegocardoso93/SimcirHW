
local SimcirHW = require '../simcirhw'

SCH = SimcirHW:new()

SCH:receive_circuit(
[===[ 
{
outputs={"Out1"}
,outputsVal={Out1="(In1 and not(In3)) or (In2 and In3)"}
,inputs={"In1","In2","In3"}
,inputsVal={In1="0",In2="1",In3="0"}
,inputSequence={In1="0,200;1,200;0,400;1,600",Infinity=true}
,pinMap={}
}
]===])

SCH:parse_circuit()

print(tostring(SCH))

SCH:parse_input_sequence()

