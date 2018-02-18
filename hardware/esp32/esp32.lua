
local Esp32 = {}

Esp32.NodeMCU_ESP32S = {
  named_pins = {
    VP  = 36, -- GPIO36
    VN  = 39, -- GPIO39
    D34 = 34, -- GPIO34
    D35 = 35, -- GPIO35
    D32 = 32, -- GPIO32
    D33 = 33, -- GPIO33
    D25 = 25, -- GPIO25
    D26 = 26, -- GPIO26
    D27 = 27, -- GPIO27
    D14 = 14, -- GPIO14
    D12 = 12, -- GPIO12
    D13 = 13, -- GPIO13
    
    D15 = 15, -- GPIO15
    D2  =  2, -- GPIO2 (builtin led)
    D4  =  4, -- GPIO4
    RX2 = 16, -- GPIO16
    TX2 = 17, -- GPIO17
    D5  =  5, -- GPIO5
    D18 = 18, -- GPIO18
    D19 = 19, -- GPIO19
    D21 = 21, -- GPIO21
    RX0 =  3, -- GPIO3
    TX0 =  1, -- GPIO1
    D23 = 22, -- GPIO22
    D22 = 23  -- GPIO23
  }
}

function Esp32.get_name()
  return "esp32"
end

function Esp32.set_board(board_name)
  Esp32.board = Esp32[board_name]
end

function Esp32.get_pin(label)
  return Esp32.board.named_pins[label]
end

return Esp32