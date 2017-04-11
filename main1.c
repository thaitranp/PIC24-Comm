#define SYSCLK  32000000UL              // 8MHz clock
#define FCY  (SYSCLK/2)
// Output pin definitions
#define LCD_RS LATBbits.LATB13 // LCD register select (0 for command, 1 for data)
#define LCD_E LATAbits.LATA4   // LCD operation enable (falling edge triggered)

#include "xc.h"
#include <libpic30.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

#pragma config FNOSC = PRI              // Oscillator Source Selection (Internal Fast RC (FRC))
#pragma config PLLMODE = DISABLED       // PLL Mode Selection (No PLL used; PLLEN bit is not available)
#pragma config IESO = ON                // Two-speed Oscillator Start-up Enable bit (Start up device with FRC, then switch to user-selected oscillator source)

// FOSC
#pragma config POSCMD = HS            // Primary Oscillator Mode Select bits (Primary Oscillator disabled)
#pragma config OSCIOFCN = OFF           // OSC2 Pin Function bit (OSC2 is clock output)
#pragma config SOSCSEL = OFF            // SOSC Power Selection Configuration bits (SOSC is used in crystal (SOSCI/SOSCO) mode)
#pragma config PLLSS = PLL_PRI          // PLL Secondary Selection Configuration bit (PLL is fed by the Primary oscillator)
#pragma config IOL1WAY = ON             // Peripheral pin select configuration bit (Allow only one reconfiguration)
#pragma config FCKSM = CSDCMD           // Clock Switching Mode bits (Both Clock switching and Fail-safe Clock Monitor are disabled)

#pragma config FWDTEN = OFF
#pragma config WINDIS = OFF             // Watchdog Timer Window Enable bit (Watchdog Timer in Non-Window mode)

#define STRLEN 12
#define _ISR_PSV __attribute__((__interrupt__, __auto_psv__))
char t;
int length;
char rcbuf[STRLEN];

void UART_init(void)
{
TRISBbits.TRISB15 = 0x001;
ANSBbits.ANSB15 = 0x000;
TRISBbits.TRISB14= 0x000;
ANSBbits.ANSB14 = 0x000;
__builtin_write_OSCCONL(OSCCON & 0xbf);
// Assign U1RX To Pin RP15
RPINR18bits.U1RXR = 0x0F;
//Assign U1TX to Pin RP14
RPOR7bits.RP14R = 0x03;
__builtin_write_OSCCONL(OSCCON | 0x40);
//data, parity and stop bit
U1MODEbits.BRGH = 0;
U1MODEbits.PDSEL = 00;
U1MODEbits.STSEL = 0;
//set baud rate
U1BRG = 103;
//interrupt config
U1STAbits.URXISEL = 00;
U1STAbits.UTXINV = 0;
//UART interrupt enable
IEC0bits.U1RXIE = 1; // interrupt on reception allowed
IEC0bits.U1TXIE = 0; // no interrupt on transmition
IEC4bits.U1ERIE = 1; // interrupts on errors allowed
IPC2bits.U1RXIP = 7; // interrupt level of reception
IPC16bits.U1ERIP = 6; // interrupt level on errors
//enable UART
U1MODEbits.UARTEN = 1;
U1STAbits.UTXEN = 1;
U1STAbits.URXEN = 1;

}

void UART_putc(char c)
{
    while (!U1STAbits.TRMT); // wait until transmit shift register is empty
    U1TXREG = c;               // write character to TXREG and start transmission
    __delay_ms(100);
}

void UART_puts(char *s)
{
    while (*s)
    {
        UART_putc(*s);     // send character pointed to by s
        s++;                // increase pointer location to the next character
    }
}

// LCD driver
void lcd_command(char c){
    LATB = (LATB & 0xFF00) | c; // Put data on LCD port
    LCD_RS = 0; // Send command
    LCD_E = 1;
    __delay_us(50);
    LCD_E = 0; // Clock on falling edge of LCD enable
}

void lcd_write_char(char c){
    LATB = (LATB & 0xFF00) | c; // Put data on LCD port
    LCD_RS = 1; // Send data
    LCD_E = 1;
    __delay_us(50);
    LCD_E = 0; // Clock on falling edge of LCD enable
}

/*
void lcd_write_str(char * s){
    char i = 0;
    while (s[i] != '\0'){
        lcd_write_char(s[i]);
        i++;
    }
}
*/

void lcd_init(void){
    TRISAbits.TRISA4 = 0;  // Set RA4 (LCD E) as output.
    TRISAbits.TRISA0 = 0;  // Set RA0 (XBee CTS) as output.
    TRISB = 0xFF00;        // Set Port B lower byte as output.
    TRISBbits.TRISB13 = 0; // Set RB13 (LCD RS) as output.
    LCD_E = 0;
    __delay_ms(8);
    lcd_command(0x38); // Function set: 8 bit interface, 2 lines, 5x8 font size
    lcd_command(0x01); // Clear display
    __delay_ms(3);
    lcd_command(0x0c); // Display ON, cursor OFF, blinking cursor OFF
    // R/W is wired low in hardware -- write only (cannot read data from LCD)
}

void _ISR_PSV _U1RXInterrupt(void)
{
   if(IFS0bits.U1RXIF)
{
    //lcd_command(0x01);
    int rcindex = 0;
    while(U1STAbits.URXDA)
    {
    t = U1RXREG;      // read received character to buffer
    rcbuf[rcindex] = t;
    rcindex ++;
    __delay_ms(50);
    UART_puts("tx");
    }
    __delay_ms(50);
    UART_puts(rcbuf);
    length = rcindex;
    rcindex = 0;
    U1STAbits.OERR = 0 ;
    IFS0bits.U1RXIF = 0;

}

 }


int main(void) {
    ANSA = 0x000;
    int i;
    lcd_init();
    INTCON2bits.GIE = 1;
    UART_init();
    while(1)

    {
    UART_puts("abcd");
    lcd_command(0x02);
    for(i=0;i<=length; i++){
        lcd_write_char(rcbuf[i]);
    }
    }

    return 0;

}