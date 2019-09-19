/**
 * PLATOTerm64 - A PLATO Terminal for the Commodore 64
 * Based on Steve Peltz's PAD
 * 
 * io_u2eth.h - Input/output functions for 1541 Ultimate II ethernet
 */

void io_send_byte_u2eth(uint8_t b);
void io_open_u2eth(void);
void io_main_u2eth(void);
void io_done_u2eth(void);
uint8_t u2eth_detect(void);
void io_connect_u2eth(void);
void io_recv_serial_flow_on_u2eth(void);
void io_recv_serial_flow_off_u2eth(void);
