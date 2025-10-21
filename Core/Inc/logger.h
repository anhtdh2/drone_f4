#ifndef INC_LOGGER_H_
#define INC_LOGGER_H_

#include <stdbool.h>
#include <stdint.h>

void logger_init(void);
void logger_log(const char* format, ...);

#endif /* INC_LOGGER_H_ */