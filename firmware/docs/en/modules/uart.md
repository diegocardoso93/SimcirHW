# UART Module
| Since  | Origin / Contributor  | Maintainer  | Source  |
| :----- | :-------------------- | :---------- | :------ |
| 2014-12-22 | [Zeroday](https://github.com/funshine) | [Zeroday](https://github.com/funshine) | [uart.c](../../../app/modules/uart.c)|

The [UART](https://en.wikipedia.org/wiki/Universal_asynchronous_receiver/transmitter) (Universal asynchronous receiver/transmitter) module allows configuration of and communication over the UART serial port.

The default setup for the console uart is controlled by build-time settings. The default uart for console is `UART0`. The default rate is 115,200 bps. In addition, auto-baudrate detection is enabled for the first two minutes
after platform boot. This will cause a switch to the correct baud rate once a few characters are received. Auto-baudrate detection is disabled when `uart.setup` is called.

For other uarts, you should call `uart.setup` and `uart.start` to get them working.

## uart.on()

Sets the callback function to handle UART events.

#### Syntax
`uart.on([id], method, [number/end_char], [function], [run_input])`

#### Parameters
- `id` uart id, default value is uart num of the console.
- `method` "data", data has been received on the UART. "error", error occurred on the UART.
- `number/end_char`. Only for event `data`.
	- if pass in a number n<255, the callback will called when n chars are received.
	- if n=0, will receive every char in buffer.
	- if pass in a one char string "c", the callback will called when "c" is encounterd, or max n=255 received.
- `function` callback function. 
  - event "data" has a callback like this: `function(data) end`
  - event "error" has a callback like this: `function(err) end`. `err` could be one of "out_of_memory", "break", "rx_error".
- `run_input` 0 or 1. Only for "data" event on console uart. If 0, input from UART will not go into Lua interpreter, can accept binary data. If 1, input from UART will go into Lua interpreter, and run.

To unregister the callback, provide only the "data" parameter.

#### Returns
`nil`

#### Example
```lua
-- when 4 chars is received.
uart.on("data", 4,
  function(data)
	print("receive from uart:", data)
	if data=="quit" then
	  uart.on("data") -- unregister callback function
	end
end, 0)
-- when '\r' is received.
uart.on("data", "\r",
  function(data)
	print("receive from uart:", data)
	if data=="quit\r" then
	  uart.on("data") -- unregister callback function
	end
end, 0)

-- uart 2
uart.on(2, "data", "\r",
  function(data)
	print("receive from uart:", data)
  end)
  
-- error handler
uart.on(2, "error",
  function(data)
	print("error from uart:", data)
  end)
```

## uart.setup()

(Re-)configures the communication parameters of the UART.

#### Syntax
`uart.setup(id, baud, databits, parity, stopbits, echo_or_pins)`

#### Parameters
- `id` uart id
- `baud` one of 300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 74880, 115200, 230400, 256000, 460800, 921600, 1843200, 3686400
- `databits` one of 5, 6, 7, 8
- `parity` `uart.PARITY_NONE`, `uart.PARITY_ODD`, or `uart.PARITY_EVEN`
- `stopbits` `uart.STOPBITS_1`, `uart.STOPBITS_1_5`, or `uart.STOPBITS_2`
- `echo_or_pins`
  - for console uart, this should be a int. if 0, disable echo, otherwise enable echo
  - for others, this is a table:
    - `tx` int. TX pin. Required
	- `rx` int. RX pin. Required
	- `cts` in. CTS pin. Optional
	- `rts` in. RTS pin. Optional
	- `tx_inverse` boolean. Inverse TX pin. Default: `false`
	- `rx_inverse` boolean. Inverse RX pin. Default: `false`
	- `cts_inverse` boolean. Inverse CTS pin. Default: `false`
	- `rts_inverse` boolean. Inverse RTS pin. Default: `false`
	- `flow_control` int. Combination of `uart.FLOWCTRL_NONE`, `uart.FLOWCTRL_CTS`, `uart.FLOWCTRL_RTS`. Default: `uart.FLOWCTRL_NONE`

#### Returns
configured baud rate (number)

#### Example
```lua
-- configure for 9600, 8N1, with echo
uart.setup(0, 9600, 8, uart.PARITY_NONE, uart.STOPBITS_1, 1)
```

```lua
uart.setup(2, 115200, 8, uart.PARITY_NONE, uart.STOPBITS_1, {tx = 16, rx = 17})
```

## uart.start()
Start the UART. You do not need to call `start()` on the console uart.

#### Syntax
`uart.start(id)`

#### Parameters
- `id` uart id, except console uart

#### Returns
Boolean. `true` if uart is started.


## uart.stop()
Stop the UART. You should not call `stop()` on the console uart.

#### Syntax
`uart.stop(id)`

#### Parameters
- `id` uart id, except console uart

#### Returns
`nil`


## uart.write()

Write string or byte to the UART.

#### Syntax
`uart.write(id, data1 [, data2, ...])`

#### Parameters
- `id` uart id
- `data1`... string or byte to send via UART

#### Returns
`nil`

#### Example
```lua
uart.write(0, "Hello, world\n")
```

