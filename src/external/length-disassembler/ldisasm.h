#pragma once

#ifndef LDISASM_H
#define LDISASM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

size_t ldisasm(const void* const address, const bool x86_64_mode);

#endif //LDISASM_H