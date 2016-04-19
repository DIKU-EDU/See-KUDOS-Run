#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "cpu.h"
#include "mem.h"
#include "exception.h"
#include "mips32.h"
#include "error.h"

memory_t* mem_init(size_t size)
{
	memory_t* memory = (memory_t*)calloc(1, sizeof(memory_t));
	uint8_t* pmem = (uint8_t*)calloc(1, size);

	if(memory == NULL || pmem == NULL) {
		printf("Could not allocate memory.\n");
		exit(1);
	}

	memory->pmem = pmem;
	memory->size_total = size;

	memory->size_kseg0 = memory->size_kseg1 = size / 8;
	memory->size_kseg2 = size / 4;
	memory->size_kuseg = size / 2;


	DEBUG("KSEG0: 0x%08x\tKSEG1: 0x%08x\tKUSEG: 0x%08x",memory->size_kseg0,
	      memory->size_kseg1, memory->size_kuseg);
	return memory;
}


exception_t mem_read(core_t *core, memory_t *mem, int32_t vaddr, uint32_t *dst,
		     mem_op_size_t op_size)
{
	/* Translated physical address */
	uint32_t paddr = vaddr_translate(vaddr);

	/* Translate to actual address*/
	uint8_t *aaddr = paddr_translate(paddr, mem);
	aaddr = aaddr;

	if(op_size == MEM_OP_BYTE) {
		*dst = GET_BIGBYTE(mem->pmem, paddr);
	} else if(op_size == MEM_OP_HALF) {
		*dst = GET_BIGHALF(mem->pmem, paddr);
	} else if(op_size == MEM_OP_WORD) {
		*dst = GET_BIGWORD(mem->pmem, paddr);
	}

	return EXC_None;
}

exception_t mem_write(core_t *core, memory_t *mem, int32_t vaddr, uint32_t src,
		      mem_op_size_t op_size)
{
	/* Translate to physical */
	uint32_t paddr = vaddr_translate(vaddr);

	/* write */
	if(op_size == MEM_OP_BYTE) {
		SET_BIGBYTE(mem->pmem, paddr, src);
	} else if(op_size == MEM_OP_HALF) {
		SET_BIGHALF(mem->pmem, paddr, src);
	} else if(op_size == MEM_OP_WORD) {
		SET_BIGHALF(mem->pmem, paddr, src);
	}

	return EXC_None;
}


uint32_t vaddr_translate(uint32_t vaddr)
{
	uint32_t paddr = 0x00;

	/* See Mips Run p. 48 */
	/* kseg2 */
	if(vaddr > KSEG2_PSTART) {
		/* TODO: NOT MAPPED YET */
		return 0x00;
	/* kseg1 */
	} else if(vaddr > KSEG1_PSTART) {
		/* Strip off first 3 bits by ANDing:
		 *	1010  (0xA)
		 * AND  0001  (0x1)
		 * ----------------
		 *      0000  (0x0) */
		paddr = vaddr & 0x1FFFFFFF;
	/* kseg0 */
	} else if(vaddr > KSEG0_PSTART) {
		/* Strip off first bit by ANDing */
		paddr = vaddr & 0x7FFFFFFF;
	/* kuseg */
	}
	if(vaddr < KSEG0_PSTART) {
		DEBUG("USER ADDR DETECTED");
		/* XXX: For now, KUSEG is located immediatelly after KUSEG0 */
		DEBUG("PADDR BEFORE: 0x%08X, KSEG0_SIZE = 0x%08X, KSEG1_SIZE = 0x%08X"
		      , paddr,  (uint32_t)KSEG0_SIZE, (uint32_t) KSEG1_SIZE);

		paddr = vaddr + KSEG0_SIZE + KSEG1_SIZE;

		DEBUG("PADDR AFTER: 0x%08X, KSEG0_SIZE = 0x%08X, KSEG1_SIZE = 0x%08X"
			, paddr, (uint32_t)KSEG0_SIZE,  (uint32_t)KSEG1_SIZE);

	}

	DEBUG("Translated 0x%08X to 0x%08X.", vaddr, paddr);
	return paddr;
}

uint8_t* paddr_translate(uint32_t paddr, memory_t *mem)
{
	/* Actual address */
	uint8_t *aaddr = NULL;

	/* KSEG2 */
	if(paddr >= KSEG0_SIZE + KSEG1_SIZE + KUSEG_SIZE) {
		/* TODO */
	/* KUSEG */
	} else if(paddr >= KSEG1_SIZE + KSEG0_SIZE) {
		/* Check if out of bounds */
		if(paddr >= KSEG1_PSTART + mem->size_kseg0 + mem->size_kseg1 +
		   mem->size_kuseg) {
			/* TODO: Exception */
			return aaddr;
		}

		/* Calculate the actual address in the simulator */
		aaddr = mem->pmem + (paddr - KSEG1_PSTART - KSEG0_PSTART -
				     KUSEG_PSTART);

	/* KSEG0 */
	} else if(paddr >= KSEG1_SIZE) {
		/* Check if out of bounds */
		if(paddr >= KSEG1_PSTART + mem->size_kseg0 + mem->size_kseg1) {
			/* TODO: Exception */
			return aaddr;
		}

		/* Calculate the actual address in the simulator */
		aaddr = mem->pmem + (paddr - KSEG1_PSTART - KSEG0_PSTART);

	/* KSEG1 */
	} else {
		/* Check if out of bounds */
		if(paddr >= KSEG1_PSTART + mem->size_kseg0) {
			/* TODO: Exception */
			return aaddr;
		}

		/* Calculate the actual address in the simulator */
		aaddr = mem->pmem + (paddr - KSEG1_PSTART);
	}


	DEBUG("Translated 0x%08X to %p.", paddr, aaddr);
	return aaddr;

}


void mem_free(memory_t *mem)
{
	free(mem->pmem);
	free(mem);
}


