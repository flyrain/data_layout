/*
 * pageMd5Comparison.c
 *
 *  Created on: Dec 18, 2011
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

extern int potential_pgd;
extern int real_pgd;
extern char *snapshot;
extern long long timeval_diff(struct timeval *difference, struct timeval *end_time,
		struct timeval *start_time);

/*determine whether a value is a valid non-empty kernel point */
int isKernelAddress(unsigned vaddr, Mem * mem) {
	unsigned kernleStartAddr;
	kernleStartAddr = 0xc0000000;
	if (vaddr > kernleStartAddr) {
		unsigned pAddr = vtop(mem->mem, mem->mem_size, mem->pgd, vaddr);
		if (pAddr > 0 && pAddr < mem->mem_size)
			return 1;
	}
	return 0;
}

//generate md5 and print virtual address and md5
void genMd5(void * startAdress, int pageSize, unsigned vaddr) {
	unsigned char md5digest[16];
	MDMem(startAdress, pageSize, md5digest);
//	printf("%x ", vaddr); //print vaddr
	MDPrint(md5digest);
	printf(" %x ", vaddr); //print vaddr
	printf("\n");
}

//generate md5 and print virtual address and md5
void genMd5WithOffset(void * startAdress, int pageSize, unsigned vaddr, unsigned offset) {
	unsigned char md5digest[16];
	MDMem(startAdress, pageSize, md5digest);
	//print md5 and vaddr
	MDPrint(md5digest);
	printf(" %x %u", vaddr, offset / 4096);
	printf("\n");
}

//find kernel code page with opcode: sti sysexit
//void findKernelCode() {
//	for (; startVirtual > startVirtualAddr - 1; startVirtual += 0x1000) {
//		//	for (; startVirtual < 0x818f0000; startVirtual += 0x1000) {
//		unsigned vAddr = startVirtual;
//
//		int rw = 0; //read or write
//		int us = 0; //use or system
//		int g = 0; //global(no move out of TLB) or not global
//		unsigned pAddr = vtopPageRW(mem->mem, mem->mem_size, mem->pgd, vAddr,
//				&rw, &us, &g);
//
//		// IS PHYSICAL ADDRESS VALID?
//		if (pAddr == -1||pAddr>mem->mem_size)
//			continue;
//
//		//collect pages which are read only, system access, and global pages
//		//	if (rw == 0 && us == 0 && g == 256) {
//		if (us == 0 && g == 256) {
//			//		printf("addr is %x\n", vAddr);
//			if (find_kernel(mem, vAddr, pageSize) == 0) {
//				kernelVirtualAddr = vAddr;
//				printf("vaddr is %x\n", vAddr);
//				//		break;
//			}
//		}
//	}
//}

extern unsigned out_pc;

extern range ranges[];
extern unsigned range_index;
extern int newstart;
cluster clusters[1 << 16];

//if there are some kernel address in the range
int containKernelAddres(cluster range, unsigned cr3address[]) {
	int ret = -1;
	int i = 0;
	int cr3No = 0;
	while (cr3address[i] != 0) {
		if (cr3address[i] >= range.start && cr3address[i] <= range.end) {
//			ret = 0;
//			break;
			cr3No++;
		}
		i++;
	}
	if (cr3No > 0)
		return cr3No;
	return ret;
}

int containKernelAddresForRange(range range, unsigned cr3address[]) {
	int ret = -1;
	int i = 0;
	while (cr3address[i] != 0) {
		if (cr3address[i] >= range.start && cr3address[i] <= range.end) {
			ret = 0;
			break;
		}
		i++;
	}
	return ret;
}

//recorde performance to file "outTime"
void recordPerformance(unsigned kdi_time, int sigGen_time, int match_time) {
	//record performance
	extern int potential_pgd_time;
	extern int real_pgd_time;
	FILE *out_time;
	out_time = fopen("outTime", "a+");
	fprintf(out_time, "%d\t%d\t%d\t%d\t%d\t%s\n", potential_pgd_time + real_pgd_time, kdi_time,
			sigGen_time, match_time,
			(potential_pgd_time + real_pgd_time + kdi_time + sigGen_time + match_time),
			snapshot);
	fclose(out_time);
}

//signatures match
void sigMatch(range range, Mem *mem, int pageSize, int dsmPages[], int *match_time) {
	int i;
	struct timeval earlier;
	struct timeval later;
	//begin match
	int osNumber = initDb();
	extern fingerprint fingerprints[FINGERPRINT_NO];
	if (gettimeofday(&earlier, NULL)) {
		perror("gettimeofday() error");
		exit(1);
	}
	int availableOs[FINGERPRINT_NO], matchCounts[FINGERPRINT_NO];
	for (i = 0; i < FINGERPRINT_NO; i++) {
		availableOs[i] = 1;
		matchCounts[i] = 0;
	}
	unsigned startVirtualAddr = range.start;
	for (; startVirtualAddr <= range.end; startVirtualAddr += 0x1000) {
		unsigned pAddr = vtop(mem->mem, mem->mem_size, mem->pgd, startVirtualAddr);
		if (pAddr == -1 || pAddr > mem->mem_size)
			continue;

		int pageIndex = pAddr / pageSize;
		if (dsmPages[pageIndex] == 1) {
			int offset = (startVirtualAddr - range.start) / 4096;
			void *startAdress = (void*) ((unsigned) mem->mem + pageIndex * pageSize);
			unsigned char md5digest[16];
			MDMem(startAdress, pageSize, md5digest);
			//	printf("%x ", vaddr); //print vaddr
			MDPrint(md5digest);
			printf("\n");
			int ret = matchByIndex(osNumber, md5digest, offset, availableOs, matchCounts);
			//			if(ret ==1)
		}
	}

	int maxIndex = -1;
	int maxMatch = 0;
	for (i = 0; i < FINGERPRINT_NO; i++) {
		if (matchCounts[i] > maxMatch) {
			maxIndex = i;
			maxMatch = matchCounts[i];
		}
	}
	if (maxMatch > 0)
		printf("Os is %s, match count is %d\n", fingerprints[maxIndex].osVersion, maxMatch);

	else
		puts("Unknown OS!");

	if (gettimeofday(&later, NULL)) {
		perror("gettimeofday() error");
		exit(1);
	}

	*match_time = timeval_diff(NULL, &later, &earlier) / 1000;
	printf("match time cost is %d milliseconds\n", *match_time);
}

//record data to file
void recordData(int allPages, unsigned cluster_index, int cr3ClusterNo, int cr3PageNo,
		unsigned finalTotalPageNo, unsigned disasPageNo, float byerate) {
	FILE *out_data;
	out_data = fopen("outData", "a+");
	fprintf(out_data, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%f\t%d\t%s\n", potential_pgd, real_pgd,
			allPages, cluster_index, cr3ClusterNo, cr3PageNo, range_index, finalTotalPageNo,
			disasPageNo, byerate, disasPageNo, snapshot);
	fclose(out_data);
}

int findClusterHasMaxCr3(unsigned cluster_index, cluster clusters[], unsigned cr3Pages[],
		int *cr3ClusterNo) {
	int i;
	int maxcr3Index = -1;
	int maxLen = 0;
	for (i = 1; i <= cluster_index; i++) {
		int cr3No = containKernelAddres(clusters[i], cr3Pages);
		if (cr3No == -1) {
			continue;
		}
		(*cr3ClusterNo)++;
		if (cr3No > maxLen) {
			maxLen = cr3No;
			maxcr3Index = i;
		}
	}

	printf("CR3 cluster: %d\n", *cr3ClusterNo);
	return maxcr3Index;
}

void findKernelCodePageByCr3(unsigned startVirtualAddr, Mem * mem, int pageSize, unsigned cr3Pages[]) {
	unsigned startVirtual = startVirtualAddr;
	int cr3PageIndex = 0;
	for (; startVirtual > startVirtualAddr - 1; startVirtual += 0x1000) {
		//	for (; startVirtual < 0x818f0000; startVirtual += 0x1000) {
		unsigned vAddr = startVirtual;

		int rw = 0; //read or write
		int us = 0; //use or system
		int g = 0; //global(no move out of TLB) or not global
		int ps = 0; //page size
		unsigned pAddr = vtopPageProperty(mem->mem, mem->mem_size, mem->pgd, vAddr, &rw, &us, &g,
				&ps);

		// IS PHYSICAL ADDRESS VALID?
		if (pAddr == -1 || pAddr > mem->mem_size)
			continue;

		//collect pages which are system access, and global pages
		if (us == 0 && g == 256) {
//			printf("r only page %x\n", vAddr);
			if (find_kernel(mem, vAddr, pageSize) == 0) {
				//record kernel address
				cr3Pages[cr3PageIndex++] = vAddr;
				printf("kernel start at %x\n", vAddr);
			}
		}
	}
}

//kernel code identification
int getClusters(unsigned startVirtualAddr, Mem * mem, int pageSize, int *allPages) {
	clusters[0].end = 0;
	int pre_rw = -1; //read or write
	int pre_us = -1; //use or system
	int pre_g = -1; //global, no move out of TLB
	int pre_ps = -1; //page size
	unsigned cluster_index = 0;
	newstart = 1;
//		int allPhysicalPages = 0;

	unsigned vAddr = startVirtualAddr;
	for (; vAddr > startVirtualAddr - 1; vAddr += 0x1000) {
		int rw = 0; //read or write
		int us = 0; //use or system
		int g = 0; //global, no move out of TLB
		int ps = 0; //page size 4M or 4k
		unsigned pAddr = vtopPageProperty(mem->mem, mem->mem_size, mem->pgd, vAddr, &rw, &us, &g,
				&ps);

//			extern FILE *pte_data;
		if (pAddr >= 0 && pAddr < mem->mem_size && us == 0 && g == 256) {
//			if (pAddr >= 0 && pAddr < mem->mem_size) {
			(*allPages)++;
//				if (pAddrs[pAddr / 4096] == 0)
//					allPhysicalPages++;
//				pAddrs[pAddr / 4096] = 1;
			//			printf("vaddr:%x,pAddr: %x, allpages is %d\n", vAddr,pAddr,allPages);
			//			fprintf(pte_data,"%08x vaddr:%x, kernel code\n",pAddr, vAddr);
		}

		//if PHYSICAL ADDRESS is not VALID, then start a new cluster
		if (pAddr < 0 || pAddr > mem->mem_size || us != 0 || g != 256) {
			if (newstart == 0) {
				clusters[cluster_index].end = vAddr - 1;
				//printf("err address end is %x %x\n", vAddr, ranges[range_index].end);
				newstart = 1;
			}
			continue;
		}

		//if any property changes, then start a new cluster
		if (rw != pre_rw || us != pre_us || g != pre_g || ps != pre_ps) {
			if (newstart == 0) {
				clusters[cluster_index].end = vAddr - 1;
				//printf("property change end is %x %x\n", vAddr, ranges[range_index].end);
				newstart = 1;
			}
		}

		//update pre properties
		pre_rw = rw;
		pre_us = us;
		pre_g = g;
		pre_ps = ps;

		//collect pages  with continuous properties;
		if (newstart) {
			clusters[++cluster_index].start = vAddr;
			clusters[cluster_index].end = vAddr + pageSize - 1;
			newstart = 0;
			if (ps == 1)
				clusters[cluster_index].pageSize = 0x400000;
			else
				clusters[cluster_index].pageSize = 0x1000;
		} else
			clusters[cluster_index].end = vAddr + pageSize - 1;
	}

	return cluster_index;
}

//kernel code identification
int kdi(unsigned startVirtualAddr, Mem * mem, int pageSize, unsigned cr3Pages[], int * kdi_time, int *allPages){
	struct timeval earlier;
	struct timeval later;
	if (gettimeofday(&earlier, NULL)) {
		perror("gettimeofday() error");
		exit(1);
	}

	//generate step 1 clusters
	unsigned cluster_index = getClusters(startVirtualAddr, mem, pageSize, allPages);

	findKernelCodePageByCr3(startVirtualAddr, mem, pageSize, cr3Pages);

	if (gettimeofday(&later, NULL)) {
		perror("gettimeofday() error");
		exit(1);
	}
	 (*kdi_time) = timeval_diff(NULL, &later, &earlier) / 1000;
	printf("step1,all pages: %d, cluster: %d,kdi time cost is %d milliseconds\n", *allPages,
			cluster_index,  (*kdi_time));
	return cluster_index;
}
/* traverse page by page */
void findReadOnlyPages(Mem * mem) {
	int i;
	int pageSize = 4 * 1024; //4k
	int totalPageNumber = mem->mem_size / (4 * 1024); //assume that every page has 4k

	int calledPages[totalPageNumber];
	int dsmPages[totalPageNumber];
	//record virtual address
	unsigned virtualAddrs[totalPageNumber];
	for (i = 0; i < totalPageNumber; i++) {
		calledPages[i] = 0;
		dsmPages[i] = 0;
		virtualAddrs[i] = 0;
	}

	unsigned cr3Pages[100];
	for (i = 0; i < 100; i++) {
		cr3Pages[i] = 0;
	}
	//start address
	unsigned startVirtualAddr = 0x80000000;
	//step 1. kdi
	int kdi_time =0;
	int allPages=0;
	int cluster_index= kdi(startVirtualAddr, mem,pageSize,cr3Pages, &kdi_time, &allPages);

	//step 2. signature generation
	struct timeval earlier;
	struct timeval later;
	if (gettimeofday(&earlier, NULL)) {
		perror("gettimeofday() error");
		exit(1);
	}
	int cr3ClusterNo = 0;
	int cr3PageNo = 0;
	ranges[0].end = 0;

	//find the cluster which has max cr3 number
	int maxcr3Index = findClusterHasMaxCr3(cluster_index, clusters, cr3Pages, &cr3ClusterNo);
	if (maxcr3Index == -1) {
		puts("Cannot find clusters have cr3.");
//		return ;
	}

	unsigned codePageNo = 0;
	newstart = 1;
	unsigned vAddr;
	for (vAddr = clusters[maxcr3Index].start; vAddr < clusters[maxcr3Index].end; vAddr += 0x1000) {
		cr3PageNo++;
		unsigned pAddr = vtop(mem->mem, mem->mem_size, mem->pgd, vAddr);
		if (vAddr == out_pc)
			code_init(mem, vAddr, pageSize, dsmPages, virtualAddrs, 1, calledPages, &codePageNo);
		else
			code_init(mem, vAddr, pageSize, dsmPages, virtualAddrs, 0, calledPages, &codePageNo);
	}
	ranges[range_index].end = clusters[maxcr3Index].end;
	ranges[range_index].len = ranges[range_index].end - ranges[range_index].start;

	//find the max range
	int max_len = 0, max_index = 0;
	for (i = 1; i <= range_index; i++) {
		if (containKernelAddresForRange(ranges[i], cr3Pages) != 0) {
			continue;
		}
		printf("start:%x, end:%x: len:%x kernel\n", ranges[i].start, ranges[i].end, ranges[i].len);
		if (ranges[i].len > max_len) {
			max_index = i;
			max_len = ranges[i].len;
		}
	}

	unsigned pAddr = vtop(mem->mem, mem->mem_size, mem->pgd, ranges[max_index].start);
	int pageIndex = pAddr / pageSize;
	char *page = (char*) ((unsigned) mem->mem + pAddr);
	code_preprocess(mem, page, ranges[max_index].len, 0x1000, ranges + max_index,
			dsmPages + pageIndex);

	printf("step2: cluster: %d\n", range_index);

	//print md5 of pages that can be disassembled
	startVirtualAddr = ranges[max_index].start;
	unsigned disasPageNo = 0;
	unsigned totalPageNo = 0;
	for (; startVirtualAddr <= ranges[max_index].end; startVirtualAddr += 0x1000) {
		totalPageNo++;
		unsigned pAddr = vtop(mem->mem, mem->mem_size, mem->pgd, startVirtualAddr);
		if (pAddr == -1 || pAddr > mem->mem_size)
			continue;

		int pageIndex = pAddr / pageSize;
		if (dsmPages[pageIndex] == 1) {
			unsigned offset = startVirtualAddr - ranges[max_index].start;
			void *startAdress = (void*) ((unsigned) mem->mem + pageIndex * pageSize);
			genMd5WithOffset(startAdress, pageSize, startVirtualAddr, offset);
			disasPageNo++;
		}
	}

	if (gettimeofday(&later, NULL)) {
		perror("gettimeofday() error");
		exit(1);
	}
	int sigGen_time = timeval_diff(NULL, &later, &earlier) / 1000;
	printf("step2: time cost is %d milliseconds\n", sigGen_time);
	float byerate = (float) ranges[max_index].disasBytes / (4096 * disasPageNo);
	printf("Success pages: %u/%u disassembled bytes rate: %f, page rate: %f\n", disasPageNo,
			totalPageNo, (float) ranges[max_index].disasBytes / (4096 * disasPageNo),
			(float) disasPageNo / totalPageNo);

	//record data;
	recordData(allPages, cluster_index, cr3ClusterNo, cr3PageNo, totalPageNo, disasPageNo, byerate);

	//begin match
	int match_time = 0;
	sigMatch(ranges[max_index], mem, pageSize, dsmPages, &match_time);

	//record performance
	recordPerformance(kdi_time, sigGen_time, match_time);
	return;
}
