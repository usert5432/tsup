#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <stdbool.h>

int deserialize_signal(const char *signal);
bool validate_id(const char *id);

#endif /* SERIALIZATION_H */
