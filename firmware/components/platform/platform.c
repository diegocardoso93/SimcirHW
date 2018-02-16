#include "platform.h"
#include "driver/console.h"
#include "driver/sigmadelta.h"
#include "driver/adc.h"
#include "driver/uart.h"
#include <stdio.h>
#include <string.h>

#include "lua.h"

#include "esp_log.h"

int platform_init (void)
{
  platform_ws2812_init();
  return PLATFORM_OK;
}


// *****************************************************************************
// GPIO subsection

int platform_gpio_exists( unsigned gpio ) { return GPIO_IS_VALID_GPIO(gpio); }
int platform_gpio_output_exists( unsigned gpio ) { return GPIO_IS_VALID_OUTPUT_GPIO(gpio); }


// ****************************************************************************
// UART

#define PLATFORM_UART_EVENT_DATA     (UART_EVENT_MAX + 1)
#define PLATFORM_UART_EVENT_OOM      (UART_EVENT_MAX + 2)
#define PLATFORM_UART_EVENT_RX       (UART_EVENT_MAX + 3)
#define PLATFORM_UART_EVENT_BREAK    (UART_EVENT_MAX + 4)

static const char *UART_TAG = "uart";

uart_status_t uart_status[NUM_UART];

extern bool uart_on_data_cb(unsigned id, const char *buf, size_t len);
extern bool uart_on_error_cb(unsigned id, const char *buf, size_t len);
task_handle_t uart_event_task_id = 0;

void uart_event_task( task_param_t param, task_prio_t prio ) {
  size_t p;
  uint16_t need_len;
  int16_t end_char;
  char ch;
  unsigned id;
  uart_status_t *us;
  uart_event_post_t *post = (uart_event_post_t *)param;
  id = post->id;
  us = & uart_status[id];
  if(post->type == PLATFORM_UART_EVENT_DATA) {
    need_len = us->need_len;
    end_char = us->end_char;
    
    for(p = 0; p < post->size; p++) {
      ch = *(post->data + p);
      us->line_buffer[us->line_position] = ch;
      us->line_position++;

      if((end_char == -1 && us->line_position == (need_len == 0? LUA_MAXINPUT : need_len))
        || (end_char >= 0 && (unsigned char)ch == (unsigned char)end_char)) {
        // us->line_buffer[us->line_position] = 0;
        uart_on_data_cb(id, us->line_buffer, us->line_position);
        us->line_position = 0;
      }
      if(us->line_position + 1 >= LUA_MAXINPUT) {
        us->line_position = 0;
      }
    }
    if(end_char == -1 && need_len == 0 && us->line_position > 0) {
      // us->line_buffer[us->line_position] = 0;
      uart_on_data_cb(id, us->line_buffer, us->line_position);
      us->line_position = 0;
    }
    
    free(post->data);
    free(post);
  } else {
    char *err;
    switch(post->type) {
      case PLATFORM_UART_EVENT_OOM:
        err = "out_of_memory";
        break;
      case PLATFORM_UART_EVENT_BREAK:
        err = "break";
        break;
      case PLATFORM_UART_EVENT_RX:
      default:
        err = "rx_error";
    }
    uart_on_error_cb(id, err, strlen(err));
  }
}

static void task_uart( void *pvParameters ){
  unsigned id = (unsigned)pvParameters;
  
  uart_event_post_t* post = NULL;
  uart_event_t event;
  
  for(;;) {
    if(xQueueReceive(uart_status[id].queue, (void * )&event, (portTickType)portMAX_DELAY)) {
      switch(event.type) {
        case UART_DATA:
          post = (uart_event_post_t*)malloc(sizeof(uart_event_post_t));
          if(post == NULL) {
            ESP_LOGE(UART_TAG, "Can not alloc memory in task_uart()");
            // reboot here?
            continue;
          }
          post->data = malloc(event.size);
          if(post->data == NULL) {
            ESP_LOGE(UART_TAG, "Can not alloc memory in task_uart()");
            post->id = id;
            post->type = PLATFORM_UART_EVENT_OOM;
          } else {
            post->id = id;
            post->type = PLATFORM_UART_EVENT_DATA;
            post->size = uart_read_bytes(id, (uint8_t *)post->data, event.size, 0);        
          }
          break;
        case UART_BREAK:
          post = (uart_event_post_t*)malloc(sizeof(uart_event_post_t));
          if(post == NULL) {
            ESP_LOGE(UART_TAG, "Can not alloc memory in task_uart()");
            // reboot here?
            continue;
          }
          post->id = id;
          post->type = PLATFORM_UART_EVENT_BREAK;
        case UART_FIFO_OVF:
        case UART_BUFFER_FULL:
        case UART_PARITY_ERR:
        case UART_FRAME_ERR:
          post = (uart_event_post_t*)malloc(sizeof(uart_event_post_t));
          if(post == NULL) {
            ESP_LOGE(UART_TAG, "Can not alloc memory in task_uart()");
            // reboot here?
            continue;
          }
          post->id = id;
          post->type = PLATFORM_UART_EVENT_RX;
          break;
        case UART_PATTERN_DET:
        default:
          ;
      }
      if(post != NULL) {
        task_post_medium( uart_event_task_id, (task_param_t)post );
        post = NULL;
      }
    }
  }
}

// pins must not be null for non-console uart
uint32_t platform_uart_setup( unsigned id, uint32_t baud, int databits, int parity, int stopbits, uart_pins_t* pins )
{
  if (id == CONSOLE_UART)
  {
    ConsoleSetup_t cfg;
    cfg.bit_rate  = baud;
    switch (databits)
    {
      case 5: cfg.data_bits = CONSOLE_NUM_BITS_5; break;
      case 6: cfg.data_bits = CONSOLE_NUM_BITS_6; break;
      case 7: cfg.data_bits = CONSOLE_NUM_BITS_7; break;
      case 8: // fall-through
      default: cfg.data_bits = CONSOLE_NUM_BITS_8; break;
    }
    switch (parity)
    {
      case PLATFORM_UART_PARITY_EVEN: cfg.parity = CONSOLE_PARITY_EVEN; break;
      case PLATFORM_UART_PARITY_ODD:  cfg.parity = CONSOLE_PARITY_ODD; break;
      default: // fall-through
      case PLATFORM_UART_PARITY_NONE: cfg.parity = CONSOLE_PARITY_NONE; break;
    }
    switch (stopbits)
    {
      default: // fall-through
      case PLATFORM_UART_STOPBITS_1:
        cfg.stop_bits = CONSOLE_STOP_BITS_1; break;
      case PLATFORM_UART_STOPBITS_1_5:
        cfg.stop_bits = CONSOLE_STOP_BITS_1_5; break;
      case PLATFORM_UART_STOPBITS_2:
        cfg.stop_bits = CONSOLE_STOP_BITS_2; break;
    }
    cfg.auto_baud = false;
    console_setup (&cfg);
    return baud;
  }
  else
  {
    int flow_control = UART_HW_FLOWCTRL_DISABLE;
    if(pins->flow_control & PLATFORM_UART_FLOW_CTS) flow_control |= UART_HW_FLOWCTRL_CTS;
    if(pins->flow_control & PLATFORM_UART_FLOW_RTS) flow_control |= UART_HW_FLOWCTRL_RTS;
    
    uart_config_t cfg = {
       .baud_rate = baud,
       .flow_ctrl = flow_control,
       .rx_flow_ctrl_thresh = UART_FIFO_LEN - 16,
    };
    
    switch (databits)
    {
      case 5: cfg.data_bits = UART_DATA_5_BITS; break;
      case 6: cfg.data_bits = UART_DATA_6_BITS; break;
      case 7: cfg.data_bits = UART_DATA_7_BITS; break;
      case 8: // fall-through
      default: cfg.data_bits = UART_DATA_8_BITS; break;
    }
    switch (parity)
    {
      case PLATFORM_UART_PARITY_EVEN: cfg.parity = UART_PARITY_EVEN; break;
      case PLATFORM_UART_PARITY_ODD:  cfg.parity = UART_PARITY_ODD; break;
      default: // fall-through
      case PLATFORM_UART_PARITY_NONE: cfg.parity = UART_PARITY_DISABLE; break;
    }
    switch (stopbits)
    {
      default: // fall-through
      case PLATFORM_UART_STOPBITS_1:
        cfg.stop_bits = UART_STOP_BITS_1; break;
      case PLATFORM_UART_STOPBITS_1_5:
        cfg.stop_bits = UART_STOP_BITS_1_5; break;
      case PLATFORM_UART_STOPBITS_2:
        cfg.stop_bits = UART_STOP_BITS_2; break;
    }
    uart_param_config(id, &cfg);
    uart_set_pin(id, pins->tx_pin, pins->rx_pin, pins->rts_pin, pins->cts_pin);
    uart_set_line_inverse(id, (pins->tx_inverse? UART_INVERSE_TXD : UART_INVERSE_DISABLE)
                                | (pins->rx_inverse? UART_INVERSE_RXD : UART_INVERSE_DISABLE)
                                | (pins->rts_inverse? UART_INVERSE_RTS : UART_INVERSE_DISABLE)
                                | (pins->cts_inverse? UART_INVERSE_CTS : UART_INVERSE_DISABLE)
                        );

    if(uart_event_task_id == 0) uart_event_task_id = task_get_id( uart_event_task );

    return baud;
  }
}


void platform_uart_send_multi( unsigned id, const char *data, size_t len )
{
  size_t i;
  if (id == CONSOLE_UART) {
      for( i = 0; i < len; i ++ ) {
        putchar (data[ i ]);
    }
  } else {
    uart_write_bytes(id, data, len);
  }
}

void platform_uart_send( unsigned id, uint8_t data )
{
  if (id == CONSOLE_UART)
    putchar (data);
  else
    uart_write_bytes(id, (const char *)&data, 1);
}

void platform_uart_flush( unsigned id )
{
  if (id == CONSOLE_UART)
    fflush (stdout);
}


int platform_uart_start( unsigned id )
{
  if (id == CONSOLE_UART)
    return 0;
  else {
    uart_status_t *us = & uart_status[id];
    
    esp_err_t ret = uart_driver_install(id, UART_BUFFER_SIZE, UART_BUFFER_SIZE, 3, & us->queue, 0);
    if(ret != ESP_OK) {
      return -1;
    }
    us->line_buffer = malloc(LUA_MAXINPUT);
    us->line_position = 0;
    if(us->line_buffer == NULL) {
      uart_driver_delete(id);
      return -1;
    }

    char pcName[6];
    snprintf( pcName, 6, "uart%d", id );
    pcName[5] = '\0';
    if(xTaskCreate(task_uart, pcName, 2048, (void*)id, ESP_TASK_MAIN_PRIO + 1, & us->taskHandle) != pdPASS) {
      uart_driver_delete(id);
      free(us->line_buffer);
      us->line_buffer = NULL;
      return -1;
    }
  return 0;
  }
}

void platform_uart_stop( unsigned id )
{
  if (id == CONSOLE_UART)
    ;
  else {
    uart_status_t *us = & uart_status[id];  
    uart_driver_delete(id);
    if(us->line_buffer) free(us->line_buffer);
    us->line_buffer = NULL;
    if(us->taskHandle) vTaskDelete(us->taskHandle);
    us->taskHandle = NULL;
  }
}

// *****************************************************************************
// Sigma-Delta platform interface

static gpio_num_t platform_sigma_delta_channel2gpio[SIGMADELTA_CHANNEL_MAX];

int platform_sigma_delta_exists( unsigned channel ) {
  return (channel < SIGMADELTA_CHANNEL_MAX);
}

uint8_t platform_sigma_delta_setup( uint8_t channel, uint8_t gpio_num )
{
#if 0
  // signal generator can't be stopped this way
  // stop signal generator
  if (ESP_OK != sigmadelta_set_prescale( channel, 0 ))
    return 0;
#endif

  // note channel to gpio assignment
  platform_sigma_delta_channel2gpio[channel] = gpio_num;

  return ESP_OK == sigmadelta_set_pin( channel, gpio_num ) ? 1 : 0;
}

uint8_t platform_sigma_delta_close( uint8_t channel )
{
#if 0
  // Note: signal generator can't be stopped this way
  // stop signal generator
  if (ESP_OK != sigmadelta_set_prescale( channel, 0 ))
    return 0;
#endif

  gpio_set_level( platform_sigma_delta_channel2gpio[channel], 1 );
  gpio_config_t cfg;
  // force pin back to GPIO
  cfg.intr_type = GPIO_INTR_DISABLE;
  cfg.mode = GPIO_MODE_OUTPUT;  // essential to switch IO matrix to GPIO
  cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
  cfg.pull_up_en = GPIO_PULLUP_ENABLE;
  cfg.pin_bit_mask = 1 << platform_sigma_delta_channel2gpio[channel];
  if (ESP_OK != gpio_config( &cfg ))
    return 0;

  // and set it finally to input with pull-up enabled
  cfg.mode = GPIO_MODE_INPUT;

  return ESP_OK == gpio_config( &cfg ) ? 1 : 0;
}

#if 0
// PWM emulation not possible, code kept for future reference
uint8_t platform_sigma_delta_set_pwmduty( uint8_t channel, uint8_t duty )
{
  uint8_t target = 0, prescale = 0;

  target = duty > 128 ? 256 - duty : duty;
  prescale = target == 0 ? 0 : target-1;

  //freq = 80000 (khz) /256 /duty_target * (prescale+1)
  if (ESP_OK != sigmadelta_set_prescale( channel, prescale ))
    return 0;
  if (ESP_OK != sigmadelta_set_duty( channel, duty-128 ))
    return 0;

  return 1;
}
#endif

uint8_t platform_sigma_delta_set_prescale( uint8_t channel, uint8_t prescale )
{
  return ESP_OK == sigmadelta_set_prescale( channel, prescale ) ? 1 : 0;
}

uint8_t IRAM_ATTR platform_sigma_delta_set_duty( uint8_t channel, int8_t duty )
{
  return ESP_OK == sigmadelta_set_duty( channel, duty ) ? 1 : 0;
}
// *****************************************************************************
// ADC

int platform_adc_exists( uint8_t adc ) { return adc < 2 && adc > 0; }

int platform_adc_channel_exists( uint8_t adc, uint8_t channel ) {
  return (adc == 1 && channel < 8);
}

uint8_t platform_adc_set_width( uint8_t adc, int bits ) {
  bits = bits - 9;
  if (bits < ADC_WIDTH_9Bit || bits > ADC_WIDTH_12Bit)
    return 0;
  if (ESP_OK != adc1_config_width( bits ))
    return 0;

  return 1;
}

uint8_t platform_adc_setup( uint8_t adc, uint8_t channel, uint8_t atten ) {
  if (adc == 1 && ESP_OK != adc1_config_channel_atten( channel, atten ))
    return 0;

  return 1;
}

int platform_adc_read( uint8_t adc, uint8_t channel ) {
  int value = -1;
  if (adc == 1) value = adc1_get_raw( channel );
  return value;
}

int platform_adc_read_hall_sensor( ) {
  int value = hall_sensor_read( );
  return value;
}
// *****************************************************************************
// I2C platform interface

#if 0
// platform functions for the IDF I2C driver
// they're currently deactivated because of https://github.com/espressif/esp-idf/issues/241
// long-term goal is to use these instead of the SW driver in the #else branch

#include "driver/i2c.h"
int platform_i2c_setup( unsigned id, uint8_t sda, uint8_t scl, uint32_t speed ) {
  i2c_config_t conf;
  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = sda;
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
  conf.scl_io_num = scl;
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
  conf.master.clk_speed = speed;
  if (ESP_OK != i2c_param_config( id, &conf ))
    return 0;

  if (ESP_OK != i2c_driver_install( id, conf.mode, 0, 0, 0 ))
    return 0;

  return 1;
}

int platform_i2c_send_start( unsigned id ) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start( cmd );
  esp_err_t ret = i2c_master_cmd_begin( id, cmd, 1000 / portTICK_RATE_MS );
  i2c_cmd_link_delete( cmd );

  return ret == ESP_OK ? 1 : 0;
}

int platform_i2c_send_stop( unsigned id ) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_stop( cmd );
  esp_err_t ret = i2c_master_cmd_begin( id, cmd, 1000 / portTICK_RATE_MS );
  i2c_cmd_link_delete( cmd );

  return ret == ESP_OK ? 1 : 0;
}

int platform_i2c_send_address( unsigned id, uint16_t address, int direction, int ack_check_en ) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  direction = ( direction == PLATFORM_I2C_DIRECTION_TRANSMITTER ) ? 0 : 1;

  i2c_master_write_byte( cmd, (uint8_t) ((address << 1) | direction ), ack_check_en );

  esp_err_t ret = i2c_master_cmd_begin( id, cmd, 1000 / portTICK_RATE_MS );
  i2c_cmd_link_delete( cmd );

  // we return ack (1=acked).
  if (ret == ESP_FAIL)
    return 0;
  else if (ret == ESP_OK)
    return 1;
  else
    return -1;
}

int platform_i2c_send_byte( unsigned id, uint8_t data, int ack_check_en ) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_write_byte( cmd, data, ack_check_en );

  esp_err_t ret = i2c_master_cmd_begin( id, cmd, 1000 / portTICK_RATE_MS );
  i2c_cmd_link_delete( cmd );

  // we return ack (1=acked).
  if (ret == ESP_FAIL)
    return 0;
  else if (ret == ESP_OK)
    return 1;
  else
    return -1;
}

int platform_i2c_recv_byte( unsigned id, int ack_val ){
  uint8_t data;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_read_byte( cmd, &data, ack_val > 0 ? 0 : 1 );

  esp_err_t ret = i2c_master_cmd_begin( id, cmd, 1000 / portTICK_RATE_MS );
  i2c_cmd_link_delete( cmd );

  return ret == ESP_OK ? data : -1;
}

#else

// platform functions for SW-based I2C driver
// they work around the issue with the IDF driver
// remove when functions for the IDF driver can be used instead

#include "driver/i2c_sw_master.h"
int platform_i2c_setup( unsigned id, uint8_t sda, uint8_t scl, uint32_t speed ){
  if (!platform_gpio_output_exists(sda) || !platform_gpio_output_exists(scl))
    return 0;

  if (speed != PLATFORM_I2C_SPEED_SLOW)
    return 0;

  i2c_sw_master_gpio_init(sda, scl);
  return 1;
}

int platform_i2c_send_start( unsigned id ){
  i2c_sw_master_start();
  return 1;
}

int platform_i2c_send_stop( unsigned id ){
  i2c_sw_master_stop();
  return 1;
}

int platform_i2c_send_address( unsigned id, uint16_t address, int direction, int ack_check_en ){
  // Convert enum codes to R/w bit value.
  // If TX == 0 and RX == 1, this test will be removed by the compiler
  if ( ! ( PLATFORM_I2C_DIRECTION_TRANSMITTER == 0 &&
           PLATFORM_I2C_DIRECTION_RECEIVER == 1 ) ) {
    direction = ( direction == PLATFORM_I2C_DIRECTION_TRANSMITTER ) ? 0 : 1;
  }

  i2c_sw_master_writeByte( (uint8_t) ((address << 1) | direction ));
  // Low-level returns nack (0=acked); we return ack (1=acked).
  return ! i2c_sw_master_getAck();
}

int platform_i2c_send_byte( unsigned id, uint8_t data, int ack_check_en ){
  i2c_sw_master_writeByte(data);
  // Low-level returns nack (0=acked); we return ack (1=acked).
  return ! i2c_sw_master_getAck();
}

int platform_i2c_recv_byte( unsigned id, int ack ){
  uint8_t r = i2c_sw_master_readByte();
  i2c_sw_master_setAck( !ack );
  return r;
}
#endif

int platform_i2c_exists( unsigned id ) { return id < I2C_NUM_MAX; }
