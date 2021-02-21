#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

int led_init();
bool led_set_state(bool state);

#ifdef __cplusplus
}
#endif