#ifndef TECLADO_H
#define TECLADO_H

void keyboard_irq(void);
char keyboard_getchar(void);
int keyboard_available(void);
int keyboard_get_state(void);
void keyboard_clear_buffer(void);

#define KB_STATE_SHIFT 0x01
#define KB_STATE_CTRL 0x02
#define KB_STATE_ALT 0x04
#define KB_STATE_CAPS 0x08

#endif
