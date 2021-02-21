#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

int clap_detection_init();
bool clap_detection_check();
void clap_detection_set_treshold(int value);

#ifdef __cplusplus
}
#endif