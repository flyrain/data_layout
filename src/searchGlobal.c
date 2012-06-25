/*
 * globalStruct.c
 *
 *  Created on: Feb 3, 2012
 *      Author: Yufei Gu
 *  Copyright : Copyright 2012 by UTD. all rights reserved. This material may
 *	 	 	 	be freely copied and distributed subject to inclusion of this
 *              copyright notice and our World Wide Web URL http://www.utdallas.edu
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "memory.h"
#include "mddriver.c"
#include <sys/times.h>
#include <stdint.h>
#define PAGE_SIZE 4096 //assume page size was 4k
#define DEPTH 3

range ranges[100];
int rangeCount;

int forwardAndBackword(int * pointerCount, int offset, unsigned topValue1, unsigned topValue2,
		Mem * mem1, Mem * mem2, int depth) {
	unsigned value1 = getMemValueByVirAddr(topValue1 + offset, mem1);
	unsigned value2 = getMemValueByVirAddr(topValue2 + offset, mem2);

	int ret1 = isWeakKernelAddress(value1, mem1);
	int ret2 = isWeakKernelAddress(value2, mem2);

	if (value1 == 0 && value2 == 0) {
//				nonPointerCount++;
	} else if (ret1 + ret2 == 0) {
//				nonPointerCount++;
	} else if (ret1 + ret2 == 1) {
		//printf("%x: %x %x %d %d\n", virAddr, value1, value2, ret1, ret2);
		return -1; //come to boundary, jump out of loop
	} else {
		if (isKernelAddress(value1) && isKernelAddress(value2)) {
//			int i;
//			for(i=0;i<rangeCount;i++){
//
//			}
//			if (value1 < range1.start || value1 > range1.end || value2 < range2.start
//					|| value2 > range2.end) {
//				if (nextLevel(value1, value2, mem1, mem2, depth) == 0)
//					return -1;
//			}
		}
		(*pointerCount)++;
	}
	return 0;
}


int nextLevel(unsigned topValue1, unsigned topValue2, Mem * mem1, Mem * mem2, int depth) {
	if (depth > DEPTH
	)
		return -1;
	depth++;

	int addrRange = 0;
	int nonPointerCount = 0;
	int pointerCount = 0;

	int offset;
	//FORWARD ONE PAGE SIZE (4096 BYTES)
	for (offset = 0; offset < PAGE_SIZE; offset += 4) {

		range range1;
		range1.start = topValue1;
		range1.end = topValue1 + offset;

		range range2;
		range2.start = topValue2;
		range2.end = topValue2 + offset;
		if (forwardAndBackword(&pointerCount, offset, topValue1, topValue2, mem1, mem2, depth)
				== -1)
			break;
	}

	addrRange = offset;
	//backward
	for (offset = -4; offset > -PAGE_SIZE; offset -= 4) {
		range range1;
		range1.start = topValue1 + offset;
		range1.end = topValue1 + addrRange;

		range range2;
		range2.start = topValue2 + offset;
		range2.end = topValue2 + addrRange;
		if (forwardAndBackword(&pointerCount, offset, topValue1, topValue2, mem1, mem2, depth)
				== -1)
			break;
	}

	return pointerCount;
}

//entrance
void searchGlobal(Mem * mem1, Mem * mem2) {
	int totalPageNumber = mem1->mem_size / PAGE_SIZE;
	unsigned startVirtualAddr = 0xc0000000;

	unsigned pageStart = startVirtualAddr;
	for (; pageStart > startVirtualAddr - 1; pageStart += 0x1000) {
		if (!isPageExist(pageStart, mem1) || !isPageExist(pageStart, mem2))
			continue;

		unsigned vAddr = pageStart;
		int j;

		for (j = 0; j < PAGE_SIZE; j += 4) {
			unsigned virAddr = vAddr + j;
			unsigned pAddr1 = vtop(mem1->mem, mem1->mem_size, mem1->pgd, virAddr);
			unsigned topValue1 = *((unsigned *) ((unsigned) mem1->mem + pAddr1));

			unsigned pAddr2 = vtop(mem2->mem, mem2->mem_size, mem2->pgd, virAddr);
			unsigned topValue2 = *((unsigned *) ((unsigned) mem2->mem + pAddr2));

			//BOTH VALUES ARE POINTERS
			if (isKernelAddress(topValue1, mem1) && isKernelAddress(topValue2, mem2)) {

				int addrRange = 0;
				int nonPointerCount = 0;
				int pointerCount = 0;

				int offset;
				//FORWARD ONE PAGE SIZE (4096 BYTES)
				for (offset = 0; offset < PAGE_SIZE; offset += 4) {
					rangeCount =0;
					ranges[rangeCount].start = topValue1;
					ranges[rangeCount].end = topValue1 + offset;
					rangeCount++;

					ranges[rangeCount].start = topValue2;
					ranges[rangeCount].end= topValue2 + offset;
					rangeCount++;

					if (forwardAndBackword(&pointerCount, offset, topValue1, topValue2, mem1, mem2,
							3) == -1)
						break;
				}

				addrRange = offset;

				//BACKWARD ONE PAGE SIZE (4096 BYTES)
				for (offset = -4; offset > -PAGE_SIZE; offset -= 4) {
					range range1;
					range1.start = topValue1 + offset;
					range1.end = topValue1 + addrRange;

					range range2;
					range2.start = topValue2 + offset;
					range2.end = topValue2 + addrRange;
					if (forwardAndBackword(&pointerCount, offset, topValue1, topValue2, mem1, mem2,
							3) == -1)
						break;
				}

				addrRange += -offset;

				if (pointerCount > 0)
					printf("%x: pointer %d  range %d\n", virAddr, pointerCount, addrRange);
			}
		}

	}
}
