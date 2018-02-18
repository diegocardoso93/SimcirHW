
local Esp32 = {}

Esp32.NodeMCU_ESP32S = {
  named_pins = {
    D0  =  19, -- GPIO16
    D1  =  21, -- GPIO5
    D2  =  2, -- GPIO4
    D3  =  3, -- GPIO0
    D4  =  4, -- GPIO2
    D5  =  5, -- GPIO14
    D6  =  6, -- GPIO12
    D7  =  7, -- GPIO13
    D8  =  8, -- GPIO15
    -- RX  =  9, -- GPIO3
    -- TX  = 10, -- GPIO1
    -- SD2 = 11, -- GPIO9
    SD3 = 12  -- GPIO10
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