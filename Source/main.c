#include "PJ_RPI.h"
#include <stdio.h>

#define GPIO_PULL       *(gpio.addr+37) // Pull up/pull down
#define GPIO_PULLCLK0   *(gpio.addr+38) // Pull up/pull down clock

int out[4] = { 26, 6, 5, 24 };
int in[4] = { 23, 22, 27, 17 };
int relais = 25;
enum States { start, enter_digits, four_digits_entered, action_open, action_lock };
int state, nextState, key;
int code[4];
int inputCode[4];
int digitsEntered = 0;
u_int8_t locked = 0;

int GetPressedKey()
{
	GPIO_SET = 1 << out[0];
	GPIO_CLR = 1 << out[1];
	GPIO_CLR = 1 << out[2];
	GPIO_CLR = 1 << out[3];
	usleep(1000);
	if (GPIO_READ(in[0]))
		return 1;
	if (GPIO_READ(in[1]))
		return 2;
	if (GPIO_READ(in[2]))
		return 3;
	if (GPIO_READ(in[3]))
		return -1;  //
	GPIO_CLR = 1 << out[0];
	GPIO_SET = 1 << out[1];
	GPIO_CLR = 1 << out[2];
	GPIO_CLR = 1 << out[3];
	usleep(1000);
	if (GPIO_READ(in[0]))
		return 4;
	if (GPIO_READ(in[1]))
		return 5;
	if (GPIO_READ(in[2]))
		return 6;
	if (GPIO_READ(in[3]))
		return -1;  //
	GPIO_CLR = 1 << out[0];
	GPIO_CLR = 1 << out[1];
	GPIO_SET = 1 << out[2];
	GPIO_CLR = 1 << out[3];
	usleep(1000);
	if (GPIO_READ(in[0]))
		return 7;
	if (GPIO_READ(in[1]))
		return 8;
	if (GPIO_READ(in[2]))
		return 9;
	if (GPIO_READ(in[3]))
		return -1;  //
	GPIO_CLR = 1 << out[0];
	GPIO_CLR = 1 << out[1];
	GPIO_CLR = 1 << out[2];
	GPIO_SET = 1 << out[3];
	usleep(1000);
	if (GPIO_READ(in[0]))
		return 10;  // Open
	if (GPIO_READ(in[1]))
		return 0;
	if (GPIO_READ(in[2]))
		return 11;  // Locked
	if (GPIO_READ(in[3]))
		return 12;  // Clear
	return -1;      //
}

int main()
{
	if(map_peripheral(&gpio) == -1) 
	{
		printf("Failed to map the physical GPIO registers into the virtual memory space.\n");
		return -1;
	}

	// define pinouts relais
	// (22) 25 -> 1
	INP_GPIO(relais);
	OUT_GPIO(relais);

	// define pinouts keypad
	// (37) 26 -> 1, (31) 6 -> 2, (29) 5 -> 3, (18) 24 -> 4, (16) 23 -> 5, (15) 22 -> 6, (13) 27 -> 7, (11) 17 -> 8
	INP_GPIO(out[0]);
	OUT_GPIO(out[0]);
	INP_GPIO(out[1]);
	OUT_GPIO(out[1]);
	INP_GPIO(out[2]);
	OUT_GPIO(out[2]);
	INP_GPIO(out[3]);
	OUT_GPIO(out[3]);

	INP_GPIO(in[0]);
	INP_GPIO(in[1]);
	INP_GPIO(in[2]);
	INP_GPIO(in[3]);

    GPIO_PULL = 1;
    usleep(1000);
    GPIO_PULLCLK0 = (1 << in[0]) | (1 << in[1]) | (1 << in[2]) | (1 << in[3]);
    usleep(1000);
    GPIO_PULL = 0;
    GPIO_PULLCLK0 = 0;

	while(1)
	{
        usleep(1000);
        // printf("Key pressed: %c\n", GetPressedKey());

        switch (state) {
            case start:
                printf("\r\nOpen the lock: Enter the 4-digit code and press 'O'\r\n");
                printf("Lock the lock: Enter a 4-digit code and press 'L'\r\n");
                nextState = enter_digits;
                digitsEntered = 0;
                inputCode[0] = inputCode[1] = inputCode[2] = inputCode[3] = -1;
                usleep(50000);
            break;

            case enter_digits:
                key = GetPressedKey();
                if (key >= 0 && key <= 9) {
                    if (digitsEntered == 0) {
                        printf("Code: ");
                    }
                    inputCode[digitsEntered - 1] = key;
                    if (digitsEntered < 4) {
                        printf("* ");
                        digitsEntered++;
                        nextState = enter_digits;
                    } else {
                        nextState = four_digits_entered;
                    }
                } else if (key == 12) {
                    printf("\rCode:      \rCode: ");
                    digitsEntered = 0;
                    inputCode[0] = inputCode[1] = inputCode[2] = inputCode[3] = -1;
                    nextState = enter_digits;
                }
            break;

            case four_digits_entered:
                key = GetPressedKey();
                if (key == 10) { // open
                    printf("O");
                    nextState = action_open;
                } else if (key == 11) { // lock
                    printf("L");
                    nextState = action_lock;
                } else if (key == 12) { // clear
                    printf("\rCode:      \rCode: ");
                    digitsEntered = 0;
                    inputCode[0] = inputCode[1] = inputCode[2] = inputCode[3] = -1;
                    nextState = enter_digits;
                }
            break;

            case action_open:
                if (locked == 1) {
                    if (code[0] == inputCode[0] && code[1] == inputCode[1] && code[2] == inputCode[2] && code[3] == inputCode[3]) {
                        printf("\r\nCode correct! Lock opened!\r\n");
                        GPIO_SET = 1 << relais;
                        locked = 0;
                        nextState = start;
                    } else {
                        printf("\r\nCode wrong! Try again!\r\n");
                        digitsEntered = 0;
                        nextState = enter_digits;
                    }
                } else {
                    printf("\r\nLock already open!\r\n");
                    nextState = start;
                }
            break;

            case action_lock:
                if (locked == 0) {
                    if (digitsEntered == 4) {
                        printf("\r\nLock locked!\r\n");
                        GPIO_CLR = 1 << relais;
                        locked = 1;
                        code[0] = inputCode[0];
                        code[1] = inputCode[1];
                        code[2] = inputCode[2];
                        code[3] = inputCode[3];
                        nextState = start;
                    } else {
                        printf("\r\nFaulty Code! Try again!\r\n");
                        digitsEntered = 0;
                        nextState = enter_digits;
                    }
                } else {
                    printf("\r\nLock already locked!\r\n");
                    nextState = start;
                }
            break;

            default:
            break;
        }

        state = nextState;
	}

	return 0;	
}
