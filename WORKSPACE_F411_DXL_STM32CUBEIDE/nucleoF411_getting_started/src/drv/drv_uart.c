/*
 * drv_uart.c
 */
#include "drv_uart.h"
#include "math.h"
#include "util.h"
#include "dynamixel.h"
#include "communication_manager.h"


int put_char1(char ch);
int put_char(char);
int putchar_stlink(char);
int puchar_zigbee(char);

uint8_t rec_buf1[1+1]="";
uint8_t rec_buf2[1+1]="";
uint8_t rec_buf6[1+1]="";
uint8_t car_received=0;
// uint8_t rec_buf[1+1]="";
char command_send;
int vitess_send;

extern void dyn_rcv_cb(uint8_t car);

//=================================================================
//	UART 1 INIT (ZIGBEE)
//=================================================================

void uart1_Init()
{
	  Uart1Handle.Instance          = USART1;

	  Uart1Handle.Init.BaudRate     = 115200;
	  Uart1Handle.Init.WordLength   = UART_WORDLENGTH_8B;
	  Uart1Handle.Init.StopBits     = UART_STOPBITS_1;
	  Uart1Handle.Init.Parity       = UART_PARITY_NONE;
	  Uart1Handle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
	  Uart1Handle.Init.Mode         = UART_MODE_TX_RX;
	  Uart1Handle.Init.OverSampling = UART_OVERSAMPLING_16;

	  HAL_UART_Init(&Uart1Handle);

	  HAL_UART_Receive_IT(&Uart1Handle, (uint8_t *)rec_buf1, 1);
}
//=================================================================
//	UART 2 INIT (STLINK UART)
//=================================================================
void uart2_Init()
{
	  Uart2Handle.Instance          = USART2;

	  Uart2Handle.Init.BaudRate     = USART2_BAUDRATE;
	  Uart2Handle.Init.WordLength   = UART_WORDLENGTH_8B;
	  Uart2Handle.Init.StopBits     = UART_STOPBITS_1;
	  Uart2Handle.Init.Parity       = UART_PARITY_NONE;
	  Uart2Handle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
	  Uart2Handle.Init.Mode         = UART_MODE_TX_RX;
	  Uart2Handle.Init.OverSampling = UART_OVERSAMPLING_16;

	  HAL_UART_Init(&Uart2Handle);

	  HAL_UART_Receive_IT(&Uart2Handle, (uint8_t *)rec_buf2, 1);
}
//=================================================================
//	UART 6 INIT ( DXL )
//=================================================================

void uart6_Init()
{
	  Uart6Handle.Instance          = USART6;

	  Uart6Handle.Init.BaudRate     =57600;//1000000;//57600;
	  Uart6Handle.Init.WordLength   = UART_WORDLENGTH_8B;
	  Uart6Handle.Init.StopBits     = UART_STOPBITS_1;
	  Uart6Handle.Init.Parity       = UART_PARITY_NONE;
	  Uart6Handle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
	  Uart6Handle.Init.Mode         = UART_MODE_TX_RX;
	  Uart6Handle.Init.OverSampling = UART_OVERSAMPLING_16;

	  HAL_UART_Init(&Uart6Handle);

	  HAL_UART_Receive_IT(&Uart6Handle, (uint8_t *)rec_buf6, 1);
}

//=================================================================
//	UART WRITE
//=================================================================

void uart_Write(char* toprint, int size)
{
#ifdef USE_USART_STLINK
	HAL_UART_Transmit(&Uart2Handle, (uint8_t *)toprint, size, 0xFFFF);
#else
	HAL_UART_Transmit(&Uart1Handle, (uint8_t *)toprint, size, 0xFFFF);
#endif
}

//=================================================================
//	PUCHAR PROTOTYPE (USED BY PRINTF FUNCTIONS)
//=================================================================
int put_char(char ch)
{
#if USE_USART_STLINK
	HAL_UART_Transmit(&Uart2Handle, (uint8_t *)&ch, 1, 0xFFFF);
#else
	HAL_UART_Transmit(&Uart1Handle, (uint8_t *)&ch, 1, 0xFFFF);
#endif
  return 0;
}

int put_char1(char ch)
{
	HAL_UART_Transmit(&Uart1Handle, (uint8_t *)&ch, 1, 0xFFFF);
  return 0;
}

int putchar_stlink(char ch)
{
	HAL_UART_Transmit(&Uart2Handle, (uint8_t *)&ch, 1, 0xFFFF);
	return 0;
}

int putchar_zigbee(char ch)
{
	HAL_UART_Transmit(&Uart1Handle, (uint8_t *)&ch, 1, 0xFFFF);
	return 0;
}


//=================================================================
//	Helper Functions and Buffer Management
//=================================================================

// Add data to the buffer
static void add_to_buffer(char data) {
    // Check if buffer is not full
    if ((rx_buf.i_push + 1) % RING_BUF_SIZE != rx_buf.i_pop) {
        // Add character to the buffer
        rx_buf.buf[rx_buf.i_push] = data;
        // Increment write index
        rx_buf.i_push = (rx_buf.i_push + 1) % RING_BUF_SIZE;
    }
}

// Clear the buffer
static void clear_buffer() {
    // Reset both read and write indices
    rx_buf.i_push = 0;
    rx_buf.i_pop = 0;
}

//=================================================================
//	UART RECEIVE CALLBACK5
//=================================================================

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
	if (UartHandle -> Instance == USART6)
	{
		HAL_UART_Receive_IT(&Uart6Handle, (uint8_t *)rec_buf6, 1);
		dxl_rcv_cb(rec_buf6[0]);
	}
	else
		if (UartHandle -> Instance == USART2)
		{
			HAL_UART_Receive_IT(&Uart2Handle, (uint8_t *)rec_buf2, 1);
		}

		else
			if (UartHandle -> Instance == USART1)
			{
		    	HAL_UART_Receive_IT(&Uart1Handle, (uint8_t *)rec_buf1, 1);
		    	char car_received= (char)rec_buf1[0];
		    	//char 'e' mean end of commande
		    	if(car_received == 'e'){
					rx_buf.buf[rx_buf.i_push] = '\0';
					command_send = process_command_data(rx_buf.buf);
		           	vitess_send = process_vitess_data(rx_buf.buf);
					clear_buffer();
		    	}else{
		    		add_to_buffer(car_received);
		    	}

		}
}


//================================================================
//				PUT STRING
//================================================================
void put_string(char* s)
{
	while(*s != '\0')
	{
		put_char(*s);
		s++;
	}
}

void put_string_stlink(char* s)
{
	while(*s != '\0')
	{
		putchar_stlink(*s);
		s++;
	}
}

void put_string_zigbee(char* s)
{
	while(*s != '\0')
	{
		putchar_zigbee(*s);
		s++;
	}
}

void sendFrame(unsigned char* s, int size)
{
	HAL_UART_Transmit(&Uart6Handle, s, size, 0xFFFF);
}
//================================================================
//				TERM_PRINTF
//================================================================

void term_printf(const char* fmt, ...)
{
	__gnuc_va_list         ap;
	char          *p;
	char           ch;
	unsigned long  ul;
	unsigned long long ull;
	unsigned long  size;
	unsigned int   sp;
	char           s[60];
	int first=0;

	va_start(ap, fmt);

	while (*fmt != '\0') {
		if (*fmt =='%') {
			size=0; sp=1;
			if (*++fmt=='0') {fmt++; sp=0;}	// parse %04d --> sp=0
			ch=*fmt;
			if ((ch>'0') && (ch<='9')) {	// parse %4d --> size=4
				char tmp[10];
				int i=0;
				while ((ch>='0') && (ch<='9')) {
					tmp[i++]=ch;
					ch=*++fmt;
				}
				tmp[i]='\0';
				size=str2num(tmp,10);
			}
			switch (ch) {
				case '%':
					put_char('%');
					break;
				case 'c':
					ch = va_arg(ap, int);
					put_char(ch);
					break;
				case 's':
					p = va_arg(ap, char *);
					put_string(p);
					break;
				case 'd':
					ul = va_arg(ap, long);
					if ((long)ul < 0) {
						put_char('-');
						ul = -(long)ul;
						//size--;
					}
					num2str(s, ul, 10, size, sp);
					put_string(s);
					break;
				case 'u':
					ul = va_arg(ap, unsigned int);
					num2str(s, ul, 10, size, sp);
					put_string(s);
					break;
				case 'o':
					ul = va_arg(ap, unsigned int);
					num2str(s, ul, 8, size, sp);
					put_string(s);
					break;
				case 'p':
					put_char('0');
					put_char('x');
					ul = va_arg(ap, unsigned int);
					num2str(s, ul, 16, size, sp);
					put_string(s);
					break;
				case 'x':
					ul = va_arg(ap, unsigned int);
					num2str(s, ul, 16, size, sp);
					put_string(s);
					break;
				case 'f':
					if(first==0){ ull = va_arg(ap, long long unsigned int); first = 1;}
					ull = va_arg(ap, long long unsigned int);
					int sign = ( ull & 0x80000000 ) >> 31;
					int m = (ull & 0x000FFFFF) ; // should be 0x007FFFFF
					float mf = (float)m ;
					mf = mf / pow(2.0,20.0);
					mf = mf + 1.0;
					int e = ( ull & 0x78000000 ) >> 23 ; // should be int e = ( ul & 0x7F800000 ) >> 23;
					e = e | (( ull & 0x000F00000 ) >> 20);
					e = e - 127;
					float f = mf*myPow(2.0,e);
					if(sign==1){ put_char('-'); }
					float2str((char*)s, f, 5);
					put_string((char*)s);
					break;

				default:
					put_char(*fmt);
			}
		} else put_char(*fmt);
		fmt++;
	}
	va_end(ap);
}
//================================================================
//				TERM_PRINTF STLINK
//================================================================

void term_printf_stlink(const char* fmt, ...)
{
	va_list        ap;
	char          *p;
	char           ch;
	unsigned long  ul;
	unsigned long long ull;
	unsigned long  size;
	unsigned int   sp;
	char           s[34];
	int 		 first=0;


	va_start(ap, fmt);
	while (*fmt != '\0') {
		if (*fmt =='%') {
			size=0; sp=1;
			if (*++fmt=='0') {fmt++; sp=0;}	// parse %04d --> sp=0
			ch=*fmt;
			if ((ch>'0') && (ch<='9')) {	// parse %4d --> size=4
				char tmp[10];
				int i=0;
				while ((ch>='0') && (ch<='9')) {
					tmp[i++]=ch;
					ch=*++fmt;
				}
				tmp[i]='\0';
				size=str2num(tmp,10);
			}
			switch (ch) {
				case '%':
					 putchar_stlink('%');
					break;
				case 'c':
					ch = va_arg(ap, int);
					putchar_stlink(ch);
					break;
				case 's':
					p = va_arg(ap, char *);
					put_string_stlink(p);
					break;
				case 'd':
					ul = va_arg(ap, long);
					if ((long)ul < 0) {
						putchar_stlink('-');
						ul = -(long)ul;
						//size--;
					}
					num2str(s, ul, 10, size, sp);
					put_string_stlink(s);
					break;
				case 'u':
					ul = va_arg(ap, unsigned int);
					num2str(s, ul, 10, size, sp);
					put_string_stlink(s);
					break;
				case 'o':
					ul = va_arg(ap, unsigned int);
					num2str(s, ul, 8, size, sp);
					put_string_stlink(s);
					break;
				case 'p':
					putchar_stlink('0');
					putchar_stlink('x');
					ul = va_arg(ap, unsigned int);
					num2str(s, ul, 16, size, sp);
					put_string_stlink(s);
					break;
				case 'x':
					ul = va_arg(ap, unsigned int);
					num2str(s, ul, 16, size, sp);
					put_string_stlink(s);
					break;
				case 'f':
					if(first==0){ ull = va_arg(ap, long long unsigned int); first = 1;}
					ull = va_arg(ap, long long unsigned int);
					int sign = ( ull & 0x80000000 ) >> 31;
					int m = (ull & 0x000FFFFF) ; // should be 0x007FFFFF
					float mf = (float)m ;
					mf = mf / pow(2.0,20.0);
					mf = mf + 1.0;
					int e = ( ull & 0x78000000 ) >> 23 ; // should be int e = ( ul & 0x7F800000 ) >> 23;
					e = e | (( ull & 0x000F00000 ) >> 20);
					e = e - 127;
					float f = mf*myPow(2.0,e);
					if(sign==1){ put_char('-'); }
					float2str((char*)s, f, 5);
					put_string_stlink((char*)s);
					break;
				default:
					putchar_stlink(*fmt);
			}
		} else putchar_stlink(*fmt);
		fmt++;
	}
	va_end(ap);
}
//=====================================================
void term_printf_zigbee(const char* fmt, ...)
{
	va_list        ap;
	char          *p;
	char           ch;
	unsigned long  ul;
	unsigned long long ull;
	unsigned long  size;
	unsigned int   sp;
	char           s[34];
	int first=0;


	va_start(ap, fmt);
	while (*fmt != '\0') {
		if (*fmt =='%') {
			size=0; sp=1;
			if (*++fmt=='0') {fmt++; sp=0;}	// parse %04d --> sp=0
			ch=*fmt;
			if ((ch>'0') && (ch<='9')) {	// parse %4d --> size=4
				char tmp[10];
				int i=0;
				while ((ch>='0') && (ch<='9')) {
					tmp[i++]=ch;
					ch=*++fmt;
				}
				tmp[i]='\0';
				size=str2num(tmp,10);
			}
			switch (ch) {
				case '%':
					 putchar_zigbee('%');
					break;
				case 'c':
					ch = va_arg(ap, int);
					putchar_zigbee(ch);
					break;
				case 's':
					p = va_arg(ap, char *);
					put_string_zigbee(p);
					break;
				case 'd':
					ul = va_arg(ap, long);
					if ((long)ul < 0) {
						putchar_zigbee('-');
						ul = -(long)ul;
						//size--;
					}
					num2str(s, ul, 10, size, sp);
					put_string_zigbee(s);
					break;
				case 'u':
					ul = va_arg(ap, unsigned int);
					num2str(s, ul, 10, size, sp);
					put_string_zigbee(s);
					break;
				case 'o':
					ul = va_arg(ap, unsigned int);
					num2str(s, ul, 8, size, sp);
					put_string_zigbee(s);
					break;
				case 'p':
					putchar_zigbee('0');
					putchar_zigbee('x');
					ul = va_arg(ap, unsigned int);
					num2str(s, ul, 16, size, sp);
					put_string_zigbee(s);
					break;
				case 'x':
					ul = va_arg(ap, unsigned int);
					num2str(s, ul, 16, size, sp);
					put_string_zigbee(s);
					break;
				case 'f':
					if(first==0){ ull = va_arg(ap, long long unsigned int); first = 1;}
					ull = va_arg(ap, long long unsigned int);
					int sign = ( ull & 0x80000000 ) >> 31;
					int m = (ull & 0x000FFFFF) ; // should be 0x007FFFFF
					float mf = (float)m ;
					mf = mf / pow(2.0,20.0);
					mf = mf + 1.0;
					int e = ( ull & 0x78000000 ) >> 23 ; // should be int e = ( ul & 0x7F800000 ) >> 23;
					e = e | (( ull & 0x000F00000 ) >> 20);
					e = e - 127;
					float f = mf*myPow(2.0,e);
					if(sign==1){ put_char('-'); }
					float2str((char*)s, f, 5);
					put_string_zigbee((char*)s);
					break;
				default:
					putchar_zigbee(*fmt);
			}
		} else putchar_zigbee(*fmt);
		fmt++;
	}
	va_end(ap);
}
