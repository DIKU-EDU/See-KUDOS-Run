#ifndef _SIM_H
#define _SIM_H

#include "cpu.h"
#include "mem.h"
#include "io.h"

typedef struct hardware {
	cpu_t* cpu;
	memory_t *mem;

	io_device_t *io;
} hardware_t;

int simulate(char *program, size_t cores, size_t mem, bool debug);

#endif /* _SIM_H */
