/*
 ============================================================================
 Name        : main.c
 Author      : Yufei Gu
 Version     :
 Copyright   : Copyright 2012 by UTD. all rights reserved. This material may
 	 	 	   be freely copied and distributed subject to inclusion of this
 	 	 	   copyright notice and our World Wide Web URL http://www.utdallas.edu
 Description : Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "memload.h"
#include "memory.h"

extern long long timeval_diff(struct timeval *difference,
                              struct timeval *end_time,
                              struct timeval *start_time);
unsigned getPgd(char *mem, int mem_size);

unsigned out_pc;
FILE *out_code;
struct timeval programStart;
char *snapshot;
char *snapshot2;
char *snapshot3;

void usage(char *prog)
{
    printf("%s snapshot\n", prog);
}

Mem *initMem(char *snapshot)
{
    struct timeval earlier;
    struct timeval later;
    if (gettimeofday(&earlier, NULL)) {
        perror("gettimeofday() error");
        exit(1);
    }
    char *mem;
    unsigned long mem_size;

    mem = mem_load(snapshot, &mem_size);
    if (mem == NULL)
        return NULL;
    else
        printf("mem '%s' load success! size is %ld\n", snapshot, mem_size);

    if (gettimeofday(&later, NULL)) {
        perror("gettimeofday() error");
        exit(1);
    }
    int loadTime = timeval_diff(NULL, &later, &earlier) / 1000;
    printf("Load mem time cost is %d milliseconds\n", loadTime);
    FILE *out_data;
    out_data = fopen("LoadMemTime", "a+");
    fprintf(out_data, "%d\t%s\n", loadTime, snapshot);
    fclose(out_data);

    //get pgd
    unsigned pgd = getPgd(mem, mem_size);
//      pgd =0x0f95a000;
//      pgd =0x15546000;

    //construct a struct Mem
    Mem *mem1 = (Mem *) malloc(sizeof(Mem));
    mem1->mem = mem;
    mem1->mem_size = mem_size;
    mem1->pgd = pgd;

    return mem1;
}

//get signature from a memory snapshot
void genSignature(char *snapshot1)
{

    //xed2_init();

    Mem *mem1 = initMem(snapshot1);
    Mem *mem2 = initMem(snapshot2);
//      Mem * mem3 = initMem(snapshot3);

    //traverse memory
    if (mem1 != NULL) {
        build_graph(mem1, mem2);

//              findGlobalStruct(mem1);
//              compareTwoSnapshot(mem1,mem2);
//              compareThreeSnapshot(mem1,mem2,mem3);
//              searchGlobal(mem1,mem2);
    }
    //free memory
    free_mem(mem1);
    free_mem(mem2);
}

/*determine a os version by memory snapshot*/
void determineOsVer(char *snapshot)
{
    xed2_init();
    Mem *mem1 = initMem(snapshot);

    //traverse memory
    if (mem1 != NULL) {
        determineOsVersion(mem1);
    }
    //free memory
    free_mem(mem1);
}


int main(int argc, char *argv[])
{
    if (argc < 1) {
        usage(argv[0]);
        return 1;
    }
    //load memory
    char *argument = argv[1];
    snapshot = argv[2];
    snapshot2 = argv[3];
//      snapshot3 = argv[4];
//      sscanf(argv[3], "%x", &out_pc);
//
//      out_code = fopen(argv[3], "w");

    int isScan = 0;
    int isGenerator = 0;
    //s is scanning and telling the os version
    char *pch = strchr(argument, 's');
    if (pch != NULL)
        isScan = 1;
    //g is generate signature
    pch = strchr(argument, 'g');
    if (pch != NULL)
        isGenerator = 1;

    struct timeval later;
    if (gettimeofday(&programStart, NULL)) {
        perror("gettimeofday() error");
        exit(1);
    }
    if (isScan == 1)
        determineOsVer(snapshot);
    else if (isGenerator == 1)
        genSignature(snapshot);

    if (gettimeofday(&later, NULL)) {
        perror("gettimeofday() error");
        exit(1);
    }
    printf("Total time cost is %lld milliseconds\n",
           timeval_diff(NULL, &later, &programStart) / 1000);
    return EXIT_SUCCESS;
}
