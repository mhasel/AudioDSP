// user_interface.h, Michael Haselberger
// Description: Declarations for the user interface state machine implemented in user_interface.c

// header guard
#ifndef __USER_INTERFACE_H__
#define __USER_INTERFACE_H__

// prevent C++ name mangling
#ifdef __cplusplus
extern "C" {
#endif
	
#include "main.h"


#define MAX_ITEM_SIZE (16)
#define MAX_ITEM_COUNT (7)
#define SUBMENU_COUNT (7)
typedef struct menu
{
	// state variables
	volatile uint16_t cnt;
	volatile uint16_t past_cnt;	
	volatile uint8_t menu_depth;
	volatile uint8_t item_selected;
	volatile uint8_t sub_menu_selected;
	volatile uint8_t show_values;
} menu_t;

void display_menu(uint8_t btn_pressed, uint8_t* mode);
	
#ifdef __cplusplus
}
#endif
#endif // __USER_INTERFACE_H__