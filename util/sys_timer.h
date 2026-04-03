#ifndef __SYS_TIMER_DEF_H__
#define __SYS_TIMER_DEF_H__

#include <stdint.h>

extern void sys_timer_init(void);
extern uint32_t sys_millis(void);

extern volatile uint32_t __uptime;

#endif /* !__SYS_TIMER_DEF_H__ */
