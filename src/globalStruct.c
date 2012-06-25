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
#include <glib.h>

unsigned getMemValueByVirAddr(unsigned virAddr, Mem * mem)
{
    unsigned pAddr = vtop(mem->mem, mem->mem_size, mem->pgd, virAddr);
    unsigned value = *((unsigned *) ((unsigned) mem->mem + pAddr));
    return value;
}

int compareSharp(unsigned vAddr1, unsigned vAddr2, Mem * mem1, Mem * mem2,
                 int level, int *matchNumber)
{
    unsigned pAddr1 = vtop(mem1->mem, mem1->mem_size, mem1->pgd, vAddr1);
    unsigned pAddr2 = vtop(mem2->mem, mem2->mem_size, mem2->pgd, vAddr2);
    //compare whole page data
    unsigned startAddr1 = pAddr1 & (~0xfff);
    unsigned startAddr2 = pAddr2 & (~0xfff);
    int i;
    int pointsNotMatch = 0;
    int pointsMatch = 0;
    for (i = 0; i < 4096; i += 4) {
        unsigned value1 =
            *((unsigned *) ((unsigned) mem1->mem + startAddr1 + i));
        unsigned value2 =
            *((unsigned *) ((unsigned) mem2->mem + startAddr2 + i));
        if ((isKernelAddress(value1, mem1)
             && (!isKernelAddress(value2, mem2)))
            || (!isKernelAddress(value1, mem1)
                && (isKernelAddress(value2, mem2)))) {
            pointsNotMatch++;
            break;
        } else if (isKernelAddress(value1, mem1)
                   && isKernelAddress(value2, mem2)) {

            if ((value1 & (~0xfff)) == (vAddr1 & (~0xfff))
                && (value2 & (~0xfff)) == (vAddr2 & (~0xfff))) {
                unsigned offset1 = value1 - (value1 & (~0xfff));
                unsigned offset2 = value2 - (value2 & (~0xfff));
                if (offset1 != offset2) {
                    pointsNotMatch++;
                    break;
                }
            } else {
                if (level < 1) {
                    if (compareSharp
                        (value1, value2, mem1, mem2, level + 1,
                         &pointsMatch) == 0)
                        pointsMatch++;
                } else {
                    pointsMatch++;
                }
            }
        }
    }
    if (pointsNotMatch <= 0 && pointsMatch > 1) {
//              *matchNumber = *matchNumber + pointsMatch;
        return 0;
    } else
        return -1;
}

//if the point value equals to each other, return 0, else return -1
int compareStruct(unsigned vAddr1, Mem * mem1, Mem * mem2, int level)
{
    unsigned pAddr1 = vtop(mem1->mem, mem1->mem_size, mem1->pgd, vAddr1);
    unsigned pAddr2 = vtop(mem2->mem, mem2->mem_size, mem2->pgd, vAddr1);
    int isNotAllZero = 0;
    int offset;
//      for (offset = -120; offset < 500; offset += 4) {
//              //cannot excess page range
//              if (pAddr1 + offset > (pAddr1 & (~0xfff)) + 0x1000 || pAddr1 + offset < (pAddr1 & (~0xfff))
//                              || pAddr2 + offset > (pAddr2 & (~0xfff)) + 0x1000
//                              || pAddr2 + offset < (pAddr2 & (~0xfff)))
//
//                      return 0;
//
//              unsigned value1 = *((unsigned *) ((unsigned) mem1->mem + pAddr1 + offset));
//              unsigned value2 = *((unsigned *) ((unsigned) mem2->mem + pAddr2 + offset));
//              if (value1 != value2) {
//                      return -1;
//              }
//              if (value1 != 0)
//                      isNotAllZero = 1;
//      }

    //compare whole page data
    unsigned startAddr1 = pAddr1 & (~0xfff);
    unsigned startAddr2 = pAddr2 & (~0xfff);

    int isLevel2 = 0;
    for (offset = 0; offset < 4096; offset += 4) {
        unsigned value1 =
            *((unsigned *) ((unsigned) mem1->mem + startAddr1 + offset));
        unsigned value2 =
            *((unsigned *) ((unsigned) mem2->mem + startAddr2 + offset));
        if (value1 != value2) {
            return -1;
        }
        if (value1 != 0)
            isNotAllZero = 1;
        //if point to self page, continue
        if (value1 >= (vAddr1 & (~0xfff))
            && value1 <= (vAddr1 & (~0xfff)) + 0x1000)
            continue;

        if (level >= 2)
            continue;

        //next level
        if (isKernelAddress(value1, mem1) && isKernelAddress(value1, mem2)) {
            if (compareStruct(value1, mem1, mem2, level + 1) == -1)
                return -1;
            else
                isLevel2 = 1;
        }
    }

    if (isNotAllZero) {
        if (level == 1 && isLevel2 == 0)
            return -1;
        return 0;
    } else
        return -1;
}

//if the page contain inVAddr have some points to soure page, return 0, else return -1
int isReturn(unsigned inVAddr, unsigned sourceAddr, Mem * mem,
             unsigned *p3OutAddr, unsigned *p1InAddr)
{
    int offset;
//      printf("inVAddr:%x \t", inVAddr);
    inVAddr = inVAddr & (~0xfff);

    for (offset = 0; offset < 4 * 1024; offset += 4) {
        //get value
        unsigned virAddr = inVAddr + offset;
        *p3OutAddr = virAddr;
        unsigned pAddr = vtop(mem->mem, mem->mem_size, mem->pgd, virAddr);
        unsigned value = *((unsigned *) ((unsigned) mem->mem + pAddr));
        *p1InAddr = value;
        if (value >= (sourceAddr & (~0xfff))
            && value <= (sourceAddr & (~0xfff)) + 0x1000)
            return 0;
    }
    return -1;
}

//
int isStepReturn(unsigned inVAddr, unsigned sourceAddr, Mem * mem)
{
    int offset;
//      printf("inVAddr:%x \t", inVAddr);
    unsigned pageStartAddr = inVAddr & (~0xfff);

    int inSelfPage = -1;
    int inSourcePage = -1;
    unsigned selfAddr = 0;
    int count2Step = 0, count3Step = 0;
    for (offset = 0; offset < 4 * 1024; offset += 4) {
        //get value
        unsigned virAddr = pageStartAddr + offset;
        unsigned pAddr = vtop(mem->mem, mem->mem_size, mem->pgd, virAddr);
        unsigned value = *((unsigned *) ((unsigned) mem->mem + pAddr));
        if ((value & (~0xfff)) == pageStartAddr) {
            selfAddr = value;
            inSelfPage = 0;
        } else if (value >= (sourceAddr & (~0xfff))
                   && value <= (sourceAddr & (~0xfff)) + 0x1000) {
            //step2
            unsigned p1 = (sourceAddr & (~0xfff));
            unsigned p2 = (inVAddr & (~0xfff));
//                      printf("p1:%x p2:%x,%x p1:%x\n", sourceAddr, inVAddr, virAddr, value);
            printf("%x:%d -> %x:%d,%d -> %x:%d\n", p1, sourceAddr - p1, p2,
                   inVAddr - p2, virAddr - p2, p1, value - p1);
            count2Step++;
        } else if (isKernelAddress(value, mem)) {
            //step3
            unsigned p3OutAddr = 0, p1InAddr = 0;
            if (isReturn(value, sourceAddr, mem, &p3OutAddr, &p1InAddr) ==
                0) {
//                              printf("p1:%x p2:%x,%x p3:%x,%x p1:%x\n", sourceAddr, inVAddr,virAddr, value,p3OutAddr,p1InAddr);
                unsigned p1 = (sourceAddr & (~0xfff));
                unsigned p2 = (inVAddr & (~0xfff));
                unsigned p3 = (value & (~0xfff));
                printf("%x:%d -> %x:%d,%d -> %x:%d,%d -> %x:%d\n", p1,
                       sourceAddr - p1, p2, inVAddr - p2, virAddr - p2, p3,
                       value - p3, p3OutAddr - p3, p1, p1InAddr - p1);
                count3Step++;
            }
        }
    }
    if (count2Step > 0 || count3Step > 0) {
        printf("2step:%d 3Step:%d\n", count2Step, count3Step);
        return count3Step + count2Step;
    }
    return -1;
}

// if one of address is kernel address,and another one is special address, return TRUE, else return FALSE
int isSpecialKernelAddress(unsigned addr1, unsigned addr2, Mem * mem1,
                           Mem * mem2)
{
    if ((addr1 == 0x00100100 || addr1 == 0x00200200)
        && (addr2 == 0x00100100 || addr2 == 0x00200200))
        return 1;
    if ((addr1 == 0x00100100 || addr1 == 0x00200200)
        && isKernelAddress(addr2, mem2))
        return 1;
    if ((addr2 == 0x00100100 || addr2 == 0x00200200)
        && isKernelAddress(addr1, mem1))
        return 1;
    return 0;
}



unsigned getValue(unsigned inVAddr, Mem * mem, int offset)
{
    unsigned virAddr = (inVAddr & (~0xfff)) + offset;
    unsigned pAddr = vtop(mem->mem, mem->mem_size, mem->pgd, virAddr);
    unsigned value = *((unsigned *) ((unsigned) mem->mem + pAddr));
    return value;
}

void printFixedFunc(int offset, unsigned inVaddr1, unsigned inVaddr2,
                    unsigned value1, unsigned sourceAddr)
{
    unsigned sourcePage = sourceAddr & (~0xfff);
    //print address
    if ((inVaddr1 & (~0xfff)) == (inVaddr2 & (~0xfff)))
        printf("%x:%d --> %x:%d,%d --> %x:%d\n", sourcePage,
               sourceAddr - sourcePage, (inVaddr1 & (~0xfff)),
               (inVaddr1 & 0xfff), offset, sourcePage,
               value1 - sourcePage);
    else
        printf("%x:%d --> %x/%x:%d,%d --> %x:%d\n", sourcePage,
               sourceAddr - sourcePage, (inVaddr1 & (~0xfff)),
               (inVaddr2 & (~0xfff)), (inVaddr1 & 0xfff), offset,
               sourcePage, value1 - sourcePage);

//print fixed function
    int offsetValue1 = offset - (inVaddr1 & 0xfff);
    char strLevel1[50];
    if (offsetValue1 >= 0) {
        sprintf(strLevel1, "*(*x + %d)", offsetValue1);
    } else {
        sprintf(strLevel1, "*(*x - %d)", -offsetValue1);
    }
    int offsetValue2 = value1 - sourceAddr;
    if (offsetValue2 >= 0) {
        printf("%x: x= %s - %d\n", sourcePage, strLevel1, offsetValue2);
    } else {
        printf("%x: x= %s + %d\n", sourcePage, strLevel1, -offsetValue2);
    }
}

int getIdenticalLoop(unsigned inVaddr1, unsigned inVaddr2,
                     unsigned sourceAddr, Mem * mem1, Mem * mem2,
                     GArray * addresses)
{
    unsigned sourcePage = sourceAddr & (~0xfff);
    unsigned pageStart1 = inVaddr1 & (~0xfff);
    unsigned pageStart2 = inVaddr2 & (~0xfff);

    int offset;
    //forward
    for (offset = 0;; offset += 4) {
        unsigned virAddr1 = inVaddr1 + offset;
        unsigned virAddr2 = inVaddr2 + offset;

        //in one page
        if (!
            (virAddr1 >= pageStart1 && virAddr1 < pageStart1 + 0x1000
             && virAddr2 >= pageStart2 && virAddr2 < pageStart2 + 0x1000))
            break;

        unsigned value1 = getMemValueByVirAddr(virAddr1, mem1);
        unsigned value2 = getMemValueByVirAddr(virAddr2, mem2);

        //if one value is kernel address and the other is not, then break
        if (value1 != value2 && value1 != 0 && value2 != 0) {
            int ret1 = isWeakKernelAddress(value1, mem1);
            int ret2 = isWeakKernelAddress(value2, mem2);
            if (ret1 + ret2 == 1) {
                //                              printf("break %x: %x %x %d %d\n", sourceAddr,value1, value2, ret1, ret2);
                break;          //come to boundary, jump out of loop
            }
        }
//              if ((value1 & (0xfff)) != (value2 & (0xfff)))
//                      continue;

        if (((value1 & (~0xfff)) == sourcePage)
            && ((value2 & (~0xfff)) == sourcePage)) {

            g_array_append_val(addresses, inVaddr1);

            printFixedFunc((virAddr1 - pageStart1), inVaddr1, inVaddr2,
                           value1, sourceAddr);

            if ((value1 & (0xfff)) != (value2 & (0xfff)))
                printf("value:%x %x diff: %d\n", value1, value2,
                       value1 - value2);
        } else {
            // three step
        }
    }

    //backward
    for (offset = 0;; offset -= 4) {
        unsigned virAddr1 = inVaddr1 + offset;
        unsigned virAddr2 = inVaddr2 + offset;

        //in one page
        if (!
            (virAddr1 >= pageStart1
             && virAddr1 < pageStart1 + 0x1000
             && virAddr2 >= pageStart2 && virAddr2 < pageStart2 + 0x1000))
            break;

        unsigned value1 = getMemValueByVirAddr(virAddr1, mem1);
        unsigned value2 = getMemValueByVirAddr(virAddr2, mem2);

        //if one value is kernel address and the other is not, then break
        if (value1 != value2 && value1 != 0 && value2 != 0) {
            int ret1 = isWeakKernelAddress(value1, mem1);
            int ret2 = isWeakKernelAddress(value2, mem2);
            if (ret1 + ret2 == 1) {
                //printf("break %x: %x %x %d %d\n", sourceAddr,value1, value2, ret1, ret2);
                break;          //come to boundary, jump out of loop
            }
        }

        if ((value1 & (0xfff)) != (value2 & (0xfff)))
            continue;

        if (((value1 & (~0xfff)) == sourcePage)
            && ((value2 & (~0xfff)) == sourcePage)) {
            //                      if (sourcePage == 0xc0040000)
            //                              printf("value:%x %x inVaddr1: %x %x\n", value1, value2, inVaddr1, inVaddr2);
            g_array_append_val(addresses, inVaddr1);
            printFixedFunc((virAddr1 - pageStart1), inVaddr1,
                           inVaddr2, value1, sourceAddr);
            //                      if ((inVaddr1 & (~0xfff)) != (inVaddr2 & (~0xfff)))
            //                              puts("diff");
        }

    }

    return -1;
}

//determine whether page exists
int isPageExist(unsigned pageStart, Mem * mem)
{
    //determine whether page exists
    int rw = 0;                 //read or write
    int us = 0;                 //use or system
    int g = 0;                  //global, no move out of TLB
    int ps = 0;                 //page size 4M or 4k
    unsigned pAddr1 =
        vtopPageProperty(mem->mem, mem->mem_size, mem->pgd, pageStart,
                         &rw,
                         &us, &g,
                         &ps);

    //if this page is system, global, keep it

    //    if (pAddr1 >= 0 && pAddr1 < mem->mem_size && us == 0 && g == 256 && rw ==0)
    if (pAddr1 >= 0 && pAddr1 < mem->mem_size && us == 0 && g == 256) {
        return 1;
    } else
        return 0;
}

unsigned findLoop(cluster cluster, Mem * mem1, Mem * mem2)
{
    GArray *addresses = g_array_new(FALSE, FALSE, sizeof(unsigned));
    unsigned pageStart = cluster.start;
    int loopCount = 0;
    for (; pageStart <= cluster.end; pageStart += 0x1000) {
        if (!isPageExist(pageStart, mem1)
            || !isPageExist(pageStart, mem2))
            continue;

        unsigned vAddr = pageStart;
        int j;

        for (j = 0; j < 4 * 1024; j += 4) {
            unsigned virAddr = vAddr + j;
            unsigned pAddr1 =
                vtop(mem1->mem, mem1->mem_size, mem1->pgd, virAddr);
            unsigned value1 =
                *((unsigned *) ((unsigned) mem1->mem + pAddr1));

            unsigned pAddr2 =
                vtop(mem2->mem, mem2->mem_size, mem2->pgd, virAddr);
            unsigned value2 =
                *((unsigned *) ((unsigned) mem2->mem + pAddr2));

            if (isKernelAddress(value1, mem1)
                && isKernelAddress(value2, mem2)) {

                if ((value1 & (0xfff)) != (value2 & (0xfff)))
                    continue;

                //if ((value1 < cluster.start || value1 > cluster.end)
                //  || (value2 < cluster.start || value2 > cluster.end))
                //  continue;

                if (((value1 & (~0xfff)) == vAddr)
                    && ((value2 & (~0xfff)) == vAddr)) {

                    //One step:value1 and value2 point to self page
                    if (value1 == value2) {
//                                              printf("%x: %d --> %d\n", virAddr, j, value1 - vAddr);
//                                              loopCount++;
                    }
                } else {
                    //Two step:
                    getIdenticalLoop(value1, value2, virAddr, mem1,
                                     mem2, addresses);
                }
            }
        }

    }

    //find the global data end
    int i = 0;
    unsigned global_data_end = cluster.end + 0x1000;
    for (i = 0; i < addresses->len; i++) {

        unsigned addr = g_array_index(addresses, unsigned, i);

        if (addr > global_data_end
            && addr - global_data_end < cluster.end - cluster.start) {
            global_data_end = addr;
            printf("global_data_end %x\n", global_data_end);
        }
    }


    global_data_end = global_data_end | 0xfff;
    printf("addresses no %d, global_data_end %x\n", addresses->len,
           global_data_end);
    g_array_free(addresses, FALSE);

    return global_data_end;
}

//determine if the page is code page by "mov ..."
int isCodePage(Mem * mem, unsigned vaddr)
{
    int res = FALSE;

    unsigned paddr = vtop(mem->mem, mem->mem_size, mem->pgd, vaddr);
    if (paddr == -1)
        return FALSE;

    int pageSize = 4096;
    int pageIndex = paddr / pageSize;

    unsigned char *page =
        (char *) ((unsigned) mem->mem + pageIndex * pageSize);

    int i;
    for (i = 0; i < pageSize - 5; i++) {
        if ((page[i] == 0x55 && page[i + 1] == 0x8b && page[i + 2] == 0xec)
            || (page[i] == 0x55 && page[i + 1] == 0x89
                && page[i + 2] == 0xe5)) {
//                      printf("code page! vaddr: %x pageIndex:%d, paddr:%x\n", vaddr, pageIndex, paddr);
            res = 1;
            break;
        }
    }

    return res;
}

/*determine whether a value is a valid non-empty kernel point */
int isKernelAddressOrZero(unsigned vaddr, Mem * mem)
{
    if (vaddr == 0)
        return 1;

    unsigned kernleStartAddr;
    kernleStartAddr = 0xc0000000;
    if (vaddr > kernleStartAddr) {
        unsigned pAddr = vtop(mem->mem, mem->mem_size, mem->pgd, vaddr);
        if (pAddr > 0 && pAddr < mem->mem_size)
            return 1;
    }
    return 0;
}

//print all pointer in some area, offset and value of pointer
GArray *print_all_pointers(cluster src_cluster, cluster target_cluster,
                           Mem * mem1, Mem * mem2)
{
    GArray *pointers = g_array_new(FALSE, FALSE, sizeof(unsigned));
    unsigned pageStart = src_cluster.start;
    int count = 0, same_count = 0;
    // prinf("src_cluster.end %x\n",src_cluster.end);

    for (; pageStart <= src_cluster.end; pageStart += 0x1000) {
        if (!isPageExist(pageStart, mem1)
            || !isPageExist(pageStart, mem2))
            continue;

        unsigned vAddr = pageStart;
        int j;

        for (j = 0; j < 4 * 1024; j += 4) {
            unsigned virAddr = vAddr + j;
            unsigned pAddr1 =
                vtop(mem1->mem, mem1->mem_size, mem1->pgd, virAddr);
            unsigned value1 =
                *((unsigned *) ((unsigned) mem1->mem + pAddr1));

            unsigned pAddr2 =
                vtop(mem2->mem, mem2->mem_size, mem2->pgd, virAddr);
            unsigned value2 =
                *((unsigned *) ((unsigned) mem2->mem + pAddr2));

            if (isKernelAddress(value1, mem1)
                && isKernelAddress(value2, mem2)) {
                if (value1 >= target_cluster.start
                    && value1 <= target_cluster.end
                    && value2 >= target_cluster.start
                    && value2 <= target_cluster.end) {
                    int same = (value1 == value2);
                    if (same == 1) {
                        same_count++;
                        g_array_append_val(pointers, value1);
                    }
                    printf("%x,%x/%x %d\n", virAddr, value1, value2, same);
                    count++;
                }
            }
        }

    }
    printf("pointer number is %d, same no. %d\n", count, same_count);

    return pointers;
}

//find the readonly area
cluster findreadonlyarea(Mem * mem, Mem * mem2)
{
    cluster src_cluster;
    unsigned pageStart = 0xc0000000;
    unsigned pageEnd = 0xffffe000;
    int readonly = 0;
    for (; pageStart <= pageEnd; pageStart += 0x1000) {

        //determine whether page exists
        int rw = 0;             //read or write
        int us = 0;             //use or system
        int g = 0;              //global, no move out of TLB
        int ps = 0;             //page size 4M or 4k
        unsigned pAddr1 =
            vtopPageProperty(mem->mem, mem->mem_size, mem->pgd,
                             pageStart,
                             &rw,
                             &us, &g,
                             &ps);

        //if this page is system, global,read only
        if (pAddr1 >= 0 && pAddr1 < mem->mem_size && us == 0
            && g == 256 && rw == 0) {
            // printf("%x readonly  readonly %d\n", pageStart,readonly);
            if (readonly == 0) {
                src_cluster.start = pageStart;
                readonly = 1;
            }
        } else {
            // printf("%x writable readonly %d\n", pageStart,readonly);
            if (readonly == 1) {
                src_cluster.end = pageStart - 0x1000;
                readonly = 0;
                break;
            }
        }
    }
    printf("read only area start %x, end %x\n", src_cluster.start,
           src_cluster.end);
    return src_cluster;
}


void compute_size(GArray * pointers)
{
    //sort the pointers
    int compare_unsigned(gpointer a, gpointer b) {
        unsigned *x = (unsigned *) a;
        unsigned *y = (unsigned *) b;
        return *x - *y;
    }

    g_array_sort(pointers, (GCompareFunc) compare_unsigned);

    //remove redundent
    GArray *data_structs = g_array_new(FALSE, FALSE, sizeof(unsigned));
    int i;
    unsigned pre_pointer = g_array_index(pointers, unsigned, 0);
    g_array_append_val(data_structs, pre_pointer);
    for (i = 1; i < pointers->len; i++) {
        unsigned curr_pointer = g_array_index(pointers, unsigned, i);
        if (curr_pointer != pre_pointer) {
            g_array_append_val(data_structs, curr_pointer);
            pre_pointer = curr_pointer;
        }
    }

    //compute size of data structure
    unsigned base_addr = g_array_index(data_structs, unsigned, 0);
    unsigned pre_offset = 0;

    for (i = 1; i < data_structs->len; i++) {
        unsigned curr_addr = g_array_index(data_structs, unsigned, i);
        unsigned offset = curr_addr - base_addr;
        int size = offset - pre_offset;
        printf("%x %x %d\n", g_array_index(data_structs, unsigned, i - 1),
               pre_offset, size);
        // printf("%x %d\n",pre_offset, size);
        pre_offset = offset;
    }
    printf("number of data structure %d\n", data_structs->len);

    g_array_free(data_structs, FALSE);

}

void build_graph(Mem * mem1, Mem * mem2)
{
    //1
    cluster src_cluster = findreadonlyarea(mem1, mem2);
    cluster targetcluster;
    targetcluster.start = src_cluster.end + 0x1000;
    //2
    targetcluster.end = findLoop(src_cluster, mem1, mem2);
    //RA TO DA
    GArray *pointers =
        print_all_pointers(src_cluster, targetcluster, mem1, mem2);

    compute_size(pointers);
    g_array_free(pointers, FALSE);

    //DA TO DA
    GArray *pointers_da =
        print_all_pointers(targetcluster, targetcluster, mem1, mem2);

    compute_size(pointers_da);
	
    g_array_free(pointers_da, FALSE);
    return;
}

void compareTwoSnapshot(Mem * mem1, Mem * mem2)
{
    //targetcluster.start = 0xc0000000;
    //targetcluster.end = 0xffffe000;


    int pageSize = 4 * 1024;    //4k bytes
    int totalPageNumber = mem1->mem_size / (4 * 1024);  //assume that every page has 4k

    //start address
    unsigned startVirtualAddr = 0xc0000000;

    //1.collect same sharpe pages and their points number
    unsigned sameSharpPages[totalPageNumber];
    unsigned pointsNumbers[totalPageNumber];
    int i;
    for (i = 0; i < totalPageNumber; i++) {
        sameSharpPages[i] = 0;
        pointsNumbers[i] = 0;
    }
    int indexSamePages = 0;

    unsigned vAddr = startVirtualAddr;
    for (; vAddr > startVirtualAddr - 1; vAddr += 0x1000) {

        int rw = 0;             //read or write
        int us = 0;             //use or system
        int g = 0;              //global, no move out of TLB
        int ps = 0;             //page size 4M or 4k
        unsigned pAddr1 =
            vtopPageProperty(mem1->mem, mem1->mem_size, mem1->pgd,
                             vAddr,
                             &rw, &us,
                             &g, &ps);

        //if the physical address not available, skip this page.
        //if this page is system, global, writable, keep it
//              if (!(pAddr1 >= 0 && pAddr1 < mem1->mem_size && us == 0 && g == 256 && rw == 2))
        if (!
            (pAddr1 >= 0 && pAddr1 < mem1->mem_size && us == 0
             && g == 256))
            continue;

        int j;
        int pointsNotMatch = 0;
        int pointsMatch = 0;
        for (j = 0; j < pageSize; j += 4) {
            unsigned virAddr = vAddr + j;
            unsigned pAddr1 =
                vtop(mem1->mem, mem1->mem_size, mem1->pgd, virAddr);
            unsigned pAddr2 =
                vtop(mem2->mem, mem2->mem_size, mem2->pgd, virAddr);

            //get value by assume its a point
            unsigned value1 =
                *((unsigned *) ((unsigned) mem1->mem + pAddr1));
            unsigned value2 =
                *((unsigned *) ((unsigned) mem2->mem + pAddr2));

            if (isSpecialKernelAddress(value1, value2, mem1, mem2)) {
                pointsMatch++;
                continue;
            }

            if (value1 != value2) {
                if ((isKernelAddress(value1, mem1)
                     && (!isKernelAddressOrZero(value2, mem2)))
                    || (!isKernelAddressOrZero(value1, mem1)
                        && isKernelAddress(value2, mem2))) {
                    pointsNotMatch++;
//                                      if (vAddr == 0xc03dc000) {
                    if (vAddr == 0xc073e000 || vAddr == 0xc070b000
                        || vAddr == 0xc0450000 || vAddr == 0xc0477000
                        || vAddr == 0xc1759000 || vAddr == 0xc1762000) {
                        printf("vaddr:%x,value1:%x,value2:%x\n",
                               virAddr, value1, value2);
                        printf("isKernel1:%d,isKernel2:%d\n",
                               isKernelAddress(value1, mem1),
                               isKernelAddress(value2, mem2));
                    }
                    continue;
//                                      if(pointsNotMatch > 1)
//                                              break;
//                                      else
//                                              continue;
                }
            }

            if (isKernelAddress(value1, mem1)
                && isKernelAddress(value2, mem2)) {
//                              pointsMatch++;
                //if one of points point to another page, compare next level
                if ((value1 & (~0xfff)) == vAddr
                    && (value2 & (~0xfff)) == vAddr) {
                    if (value1 != value2) {
                        pointsNotMatch++;
                        break;
                    }
                } else {
                    pointsMatch++;
                    //
//                                      int matchCountIn = 0;
//                                      if (compareSharp(value1, value2, mem1, mem2, 0, &matchCountIn) == 0)
//                                              pointsMatch++;
                }
            }
        }

        if (pointsNotMatch > 1) {
            //break, may discontinuous
            printf("%x: break! no match count:%d\n", vAddr,
                   pointsNotMatch);
            sameSharpPages[indexSamePages] = vAddr;
            pointsNumbers[indexSamePages] = 0;
            indexSamePages++;
        } else if (pointsMatch > 0) {
            printf("%x: pointsMatch %d\n", vAddr, pointsMatch);
            sameSharpPages[indexSamePages] = vAddr;
            pointsNumbers[indexSamePages] = pointsMatch;
            indexSamePages++;
        }

    }

    //2. find the largest point number cluster
    cluster clusters[3000];
    int clusterIndex = -1;
    int isNewCluster = 0;
    //divided by 4M, and get the lowest group
    for (i = 0; i < indexSamePages; i++) {
        if (isCodePage(mem1, sameSharpPages[i]) == 1) {
            isNewCluster = 1;
            continue;
        }
        //if break;
        if (pointsNumbers[i] == 0) {
            isNewCluster = 1;
            continue;
        }
//              if ((i > 0 && sameSharpPages[i] - sameSharpPages[i - 1] > 0x10000)
//                              || sameSharpPages[i] >= clusters[clusterIndex].start + 0x400000
//                              || clusterIndex == -1 || isNewCluster == 1) {
        if (clusterIndex == -1 || isNewCluster == 1) {
            //new cluster
            isNewCluster = 0;
            clusterIndex++;
            clusters[clusterIndex].start = sameSharpPages[i];
            clusters[clusterIndex].end = clusters[clusterIndex].start;
            clusters[clusterIndex].count = 1;
            clusters[clusterIndex].pointsNumber = pointsNumbers[i];
        } else {
            clusters[clusterIndex].end = sameSharpPages[i];
            clusters[clusterIndex].count++;
            clusters[clusterIndex].pointsNumber += pointsNumbers[i];
        }
    }

    //print the cluster, and find the max point number cluster
    unsigned maxPointNumber = 0;
    int maxPageNum = 0;
    int maxIndex = -1;
    for (i = 0; i < clusterIndex + 1; i++) {
        if (clusters[i].pointsNumber > maxPointNumber) {
            maxPointNumber = clusters[i].pointsNumber;
            maxIndex = i;
        }
        printf
            ("cluster %d: %x -- %x  page count: %d point number: %d\n",
             i, clusters[i].start, clusters[i].end, clusters[i].count,
             clusters[i].pointsNumber);
    }

    printf("MAX cluster: %x -- %x  page count: %d point number: %d\n",
           clusters[maxIndex].start, clusters[maxIndex].end,
           clusters[maxIndex].count, clusters[maxIndex].pointsNumber);

    //3.find loop
    //findLoop(clusters[maxIndex], mem1, mem2);

    return;
}

//whether value weak kernel address? 0x100100, 0x200200, 0, normal kernel address(such as 0xc042c340)
//0x74737461 refer to timer.h  #define TIMER_ENTRY_STATIC       ((void *) 0x74737461)
int isWeakKernelAddress(unsigned value, Mem * mem)
{
    if (value == 0x00100100 || value == 0x00200200
        || value == 0x74737461 || value == 0
        || isKernelAddress(value, mem))
        return 1;
    else
        return 0;
}

void compareThreeSnapshot(Mem * mem1, Mem * mem2, Mem * mem3)
{
    int pageSize = 4 * 1024;    //assume page size was 4k
    int totalPageNumber = mem1->mem_size / pageSize;
    unsigned startVirtualAddr = 0xc0000000;     // start address

    //1.collect same sharpe pages and their points number
    unsigned sameSharpPages[totalPageNumber];
    unsigned pointsNumbers[totalPageNumber];
    int i;
    for (i = 0; i < totalPageNumber; i++) {
        sameSharpPages[i] = 0;
        pointsNumbers[i] = 0;
    }
    int indexSamePages = 0;

    unsigned vAddr = startVirtualAddr;
    for (; vAddr > startVirtualAddr - 1; vAddr += 0x1000) {

        if (!isPageExist(vAddr, mem1) || !isPageExist(vAddr, mem2)
            || !isPageExist(vAddr, mem2))
            continue;

//              printf("page vir address:%x\n", vAddr);
        int j;
        int pointsNotMatch = 0;
        int pointsMatch = 0;
        for (j = 0; j < pageSize; j += 4) {
            unsigned virAddr = vAddr + j;
            //get values
            unsigned value1 = getMemValueByVirAddr(virAddr, mem1);
            unsigned value2 = getMemValueByVirAddr(virAddr, mem2);
            unsigned value3 = getMemValueByVirAddr(virAddr, mem3);
//                      if (vAddr == 0xc5c2a000)
//                              printf("value1,%x %x %x\n", value1, value2, value3);
            if (value1 == 0 && value2 == 0 && value3 == 0)
                continue;

            if (isWeakKernelAddress(value1, mem1)
                && isWeakKernelAddress(value2, mem2)
                && isWeakKernelAddress(value3, mem3)) {
//                              printf("weak value: %x %x %x\n", value1, value2, value3);
                if (isKernelAddress(value1, mem1)
                    && isKernelAddress(value2, mem2)
                    && isKernelAddress(value3, mem3)) {
                    //if one of points point to another page, compare next level
                    if ((value1 & (~0xfff)) == vAddr
                        && (value2 & (~0xfff)) == vAddr
                        && (value3 & (~0xfff)) == vAddr) {
                        if (!(value1 == value2 && value2 == value3)) {
                            pointsNotMatch++;
                            printf("offset diff: %x %x %x\n", value1,
                                   value2, value3);
                            break;
                        }
                    } else {
                        if ((value1 & 0xfff) == (value2 & 0xfff)
                            && (value2 & 0xfff) == (value3 & 0xfff)) {
                            pointsMatch++;
                            continue;
                        }
                    }
                } else {
//                                      pointsMatch++;
//                                      continue;
                }
            }

            if (value1 != value2 || value1 != value3) {
                // one Zero

                // two Zero

                // none Zero
                if (value1 != 0 && value2 != 0 && value3 != 0) {
//                                      printf("value: %x %x %x\n", value1, value2, value3);
                    int ret1 = isWeakKernelAddress(value1, mem1);
                    int ret2 = isWeakKernelAddress(value2, mem2);
                    int ret3 = isWeakKernelAddress(value3, mem3);
//                                      printf("%x ret: %d %d %d\n", virAddr, ret1, ret2, ret3);
                    if (ret1 + ret2 + ret3 == 1) {
                        printf("value: %x %x %x\n", value1, value2,
                               value3);
                        printf("%x ret: %d %d %d\n", virAddr, ret1,
                               ret2, ret3);
                    }
                    if (ret1 + ret2 + ret3 == 2) {
                        printf("%x: %x %x %x\n", virAddr, value1,
                               value2, value3);
                        printf("%x ret: %d %d %d\n", virAddr, ret1,
                               ret2, ret3);
                        pointsNotMatch++;
                        break;
                    }
                }
            }
        }

        if (pointsNotMatch > 0) {
            //break, may discontinuous
            printf("%x: break! no match count:%d\n", vAddr,
                   pointsNotMatch);
            sameSharpPages[indexSamePages] = vAddr;
            pointsNumbers[indexSamePages] = 0;
            indexSamePages++;
        } else if (pointsMatch > 0) {
            printf("%x: pointsMatch %d\n", vAddr, pointsMatch);
            sameSharpPages[indexSamePages] = vAddr;
            pointsNumbers[indexSamePages] = pointsMatch;
            indexSamePages++;
        }

    }

    //2. find the largest point number cluster
    cluster clusters[10000];
    int clusterIndex = -1;
    int isNewCluster = 0;
    //divided by 4M, and get the lowest group
    for (i = 0; i < indexSamePages; i++) {
        if (isCodePage(mem1, sameSharpPages[i]) == 1) {
            isNewCluster = 1;
            continue;
        }
        //if break;
        if (pointsNumbers[i] == 0) {
            isNewCluster = 1;
            continue;
        }
//              if ((i > 0 && sameSharpPages[i] - sameSharpPages[i - 1] > 0x10000)
//                              || sameSharpPages[i] >= clusters[clusterIndex].start + 0x400000
//                              || clusterIndex == -1 || isNewCluster == 1) {
        if (clusterIndex == -1 || isNewCluster == 1) {
            //new cluster
            isNewCluster = 0;
            clusterIndex++;
            clusters[clusterIndex].start = sameSharpPages[i];
            clusters[clusterIndex].end = clusters[clusterIndex].start;
            clusters[clusterIndex].count = 1;
            clusters[clusterIndex].pointsNumber = pointsNumbers[i];
        } else {
            clusters[clusterIndex].end = sameSharpPages[i];
            clusters[clusterIndex].count++;
            clusters[clusterIndex].pointsNumber += pointsNumbers[i];
        }
    }

    //print the cluster, and find the max point number cluster
    unsigned maxPointNumber = 0;
    int maxPageNum = 0;
    int maxIndex = -1;
    for (i = 0; i < clusterIndex + 1; i++) {
        if (clusters[i].pointsNumber > maxPointNumber) {
            maxPointNumber = clusters[i].pointsNumber;
            maxIndex = i;
        }
        printf("cluster %d: %x -- %x  page count: %d point number: %d\n",
               i, clusters[i].start, clusters[i].end, clusters[i].count,
               clusters[i].pointsNumber);
    }

    printf("MAX cluster: %x -- %x  page count: %d point number: %d\n",
           clusters[maxIndex].start, clusters[maxIndex].end,
           clusters[maxIndex].count, clusters[maxIndex].pointsNumber);

    //3.find loop
    //findLoop(clusters[maxIndex], mem1, mem2);

    return;
}
