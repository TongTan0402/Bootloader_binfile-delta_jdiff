#include <stdio.h>
#include <stdint.h>

typedef struct __attribute__((__packed__))
{
	uint16_t 	start;
	uint8_t 	type_message;
	uint16_t 	length;					
	uint8_t 	data[12]; 
	uint8_t 	check_sum;
} FSM_Frame_s;

FSM_Frame_s frame;

int main()
{
  printf("%x - %x - %x - %x\n", &frame.start, &frame.type_message, &frame.length, &frame.data);
}