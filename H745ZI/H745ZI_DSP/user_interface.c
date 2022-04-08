// user_interface.c, Michael Haselberger
// Description: This file contains all functions relevant to the user menu state machine. 

#include "user_interface.h"
static enum menu_levels
{
	MENU_PT = 0,
	MENU_DELAY,
	MENU_OD,
	MENU_FUZZ,
	MENU_TREM,
	MENU_RM
} menu_levels;
	
// allows use of fx handles from main.c
extern delay_handle_t delay_handle;
extern fuzz_handle_t fuzz_handle;
extern overdrive_handle_t overdrive_handle;
extern tremolo_handle_t tremolo_handle;
extern ring_mod_handle_t ring_mod_handle;

/******************************************************************************
* Function Name: confirm_value
*******************************************************************************
* Summary:
*  When the push button on the encoder is pressed when in value selection state,
*  this function gets the currently selected mode and parameter from menu struct 
*  and call the appropriate effect update function. parameter values are passed as
*  values from 0 to 100, so the parameters need to be scaled before passing them on.
*
* Parameters:
*  1. menu_t* menu					- Struct containing menu state machine variables.
* 
* Return:
*  None.							- Can be improved by returning error codes from update functions, if applicable.
*									  However, since no error handling is implemented as of now, 
*									  this function returns void.
*
******************************************************************************/
#pragma optimize_for_speed
void confirm_value(menu_t* menu)
{
	switch (menu->sub_menu_selected)
	{
	case MENU_DELAY:
		if (menu->item_selected == 1)
			delay_update(&delay_handle, DELAY, MAX_DELAY_TIME * ((float32_t)menu->cnt / 100.0f));
		// feedback and blend both have same value range. delay fx enums integer values are 1 below item_selected equivalent
		else
			delay_update(&delay_handle, menu->item_selected - 1, ((float32_t)menu->cnt / 100.0f));
		break;
	case MENU_OD:
		overdrive_update(&overdrive_handle, 0.4f * ((float32_t)menu->cnt / 100.0f));
		break;
	case MENU_FUZZ:
		if (menu->item_selected == 1)
			fuzz_update(&fuzz_handle, GAIN, 18.0f * ((float32_t)menu->cnt / 100.0f));
		else
			fuzz_update(&fuzz_handle, MIX, ((float32_t)menu->cnt / 100.0f));
		break;
	case MENU_TREM:
		tremolo_update(&tremolo_handle, menu->item_selected - 1, ((float32_t)menu->cnt / 100.0f));
		break;
	case MENU_RM:
		if (menu->item_selected != 3)
			// update selected parameter. modulation type stays the same
			ring_mod_update(&ring_mod_handle, menu->item_selected - 1, NO_CHANGE, ((float32_t)menu->cnt / 100.0f));
		else
		{	
			// updating the modulation type needs some differentiation. due to the simplified UI and only displaying values
			// as 0 to 100, modulation type is going to be defined as certain value windows.
			// the parameter value is passed as 255 to trigger the default case in the update function -> stays as it is
			if (menu->cnt < 33)
				ring_mod_update(&ring_mod_handle, menu->item_selected - 1, SINE, 255);
			else if (menu->cnt < 66)
				ring_mod_update(&ring_mod_handle, menu->item_selected - 1, TRIANGLE, 255);
			else				
				ring_mod_update(&ring_mod_handle, menu->item_selected - 1, SQUARE, 255);				
		}
		break;
	}
}

/******************************************************************************
* Function Name: display_menu
*******************************************************************************
* Summary:
*  This function handles the user menu state machine. It is responsible for displaying the correct
*  strings on the LCD and parsing user input button presses to navigate the menu and update 
*  effect modes/parameters.
*
* Parameters:
*  1. uint8_t btn_pressed			  - Push button interrupt flag. Will be 1 if an interrupt has occured, 0 otherwise.
*  2. uint8_t* mode					  - Pointer to the variable that's responsible for effect selection.
* 
* Return:
*  None.
*
******************************************************************************/
#pragma optimize_for_speed
void display_menu(uint8_t btn_pressed, uint8_t* mode)
{    
	// group initializer for all menu_t members. will only be initialized once upon first function call.
	static menu_t menu = { 0 };
	// menu entries
	static char menus[SUBMENU_COUNT][MAX_ITEM_COUNT][MAX_ITEM_SIZE] = 
	{ 
		// top level
		{"Pass-through", "Delay", "Overdrive", "Fuzz", "Tremolo", "Ring Modulator" },
		// pass through
		{ "Start", "BACK" },
		// delay
		{ "Start", "Delay", "Feedback", "Blend", "BACK" },
		// overdrive
		{ "Start", "Threshold", "BACK" },
		// fuzz
		{ "Start", "Gain", "Mix", "BACK" },
		// tremolo
		{ "Start", "Rate", "Depth", "BACK" },
		// ring mod
		{ "Start", "Rate", "Blend", "Type", "BACK" }		
	};
	
	// clear screen
	lcd_clear();	
	
	char c[16];
    
	// read counter value from timer in encoder mode
	menu.cnt = TIM2->CNT;
	
	// if the button was pressed, go to deeper menu level
	if (btn_pressed)
	{
		uint8_t size;
		// menu is implemented as a state machine, where the current menu depth acts as the state variable
		switch (menu.menu_depth)
		{
		// top level
		case 0:			
			menu.sub_menu_selected = menu.item_selected;
			menu.menu_depth++;
			break;
		// sub items
		case 1: 
			size = ARRAY_SIZE(menus[menu.sub_menu_selected][menu.item_selected]);
			// check if last entry is selected -> go back
			if (menu.item_selected == size - 1)
			{
				menu.menu_depth--;		
				// reset item selection and timer-counter
				menu.item_selected = 0;
				TIM2->CNT = 0;
			}
			// check if first entry is selected -> start
			else if (menu.item_selected == 0)
			{
				// write sub menu selected index to mode pointer from main.c
				// these values are the same as the FXMODE enums from fx_lib.h
				*mode = menu.sub_menu_selected;				
			}
			// the item selected is a parameter setting: show counter value on second line
			else
			{
				// this flag controls if the counter value (=parameter %) is displayed. it also switches
				// the counter value having an impact on menu item selection or value selection
				menu.show_values = 1;	
				menu.menu_depth++;
				// set value to 50. this could be improved by reading out the current
				// value of the selected parameter from the fx lib in the future. for now, defaults to 50
				TIM2->CNT = 50;
			}
			break;
		// confirm selected value, no longer display value.
		case 2:
			{
				confirm_value(&menu);
				menu.show_values = 0;					
			}
			break;
		default:			
			menu.menu_depth--;
			break;
		}

		btn_pressed = 0;
	}
	
	// check if/how much the encoder has been rotated
	if (menu.cnt != menu.past_cnt)
	{
		// if this flag is set, encoder rotation controls value, not item
		if (!menu.show_values)
		{			
			int16_t change = menu.cnt - menu.past_cnt;	
			menu.item_selected = (change > 0)
					? menu.item_selected++
					: menu.item_selected--;
			
			// modulo the item selected variable by the item count of the current sub-menu to avoid overflow
			menu.item_selected %= ARRAY_SIZE(menus[menu.sub_menu_selected]);			
		}
		else
		{			
			// convert counter value to char
			char val[3];
			uint8_t temp = menu.cnt;
			for (int i = 2; i >= 0; i--)
			{
				// taking mod10 of any number results in the last digit of that number
				// adding '0' converts integer to corresponding ASCII value
				val[i] = temp % 10 + '0';
				// divide number by 10 to drop last digit
				temp /= 10;
			}
			
			// put cursor on second row
			lcd_put_cursor(1, 0);
			// write count value to display
			lcd_send_string(val);
			// return cursor to first row
			lcd_put_cursor(0, 0);
		}
		
		// iterate over innermost char array
		for (int i = 0; i < ARRAY_SIZE(menus[menu.sub_menu_selected][menu.item_selected]); i++) 
		{
			c[i] = menus[menu.sub_menu_selected][menu.item_selected][i];
		}
		
		lcd_send_string(c);
	}
	
	menu.past_cnt = menu.cnt; 
}
