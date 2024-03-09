#ifndef _WS2812_H
#define _WS2812_H

#include <linux/printk.h>

#define WS2812_LOG_PREFIX 	"[ws2812]: "
#define WS2812_TURN_ON		1
#define WS2812_TURN_OFF		0
#define WS2812_CODE0		0b11000000
#define WS2812_CODE1		0b11111000
#define WS2812_DEFAULT_STRIP_LEN 8

#define WS2812_DEBUG_MODE   0

#define WS2812_MEMORY_LIMIT_B	1024

#define ws2812_error(msg, ...)      pr_err(WS2812_LOG_PREFIX msg, ##__VA_ARGS__)
#define ws2812_warning(msg, ...)    pr_warn(WS2812_LOG_PREFIX msg, ##__VA_ARGS__)
#define ws2812_info(msg, ...)       pr_info(WS2812_LOG_PREFIX msg, ##__VA_ARGS__)
#define ws2812_debug(msg, ...)      pr_debug(WS2812_LOG_PREFIX msg, ##__VA_ARGS__)

#endif /* _WS2812_H */
