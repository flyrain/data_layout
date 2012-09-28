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
#include <sys/times.h>
#include <stdint.h>
#include <glib.h>

GArray *compute_size(GArray * pointers, Mem * mem);

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

//find the loop between readonly area and global data area
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

            if (isKernelAddr(value1, mem1)
                && isKernelAddr(value2, mem2)) {

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

    return isKernelAddr(vaddr, mem);
}

//get all address directory from binary code 
GArray *get_all_addresses(cluster src_cluster, cluster target_cluster,
                          Mem * mem1, Mem * mem2)
{
    GArray *pointers = g_array_new(FALSE, FALSE, sizeof(unsigned));
    GArray *pointers2 = g_array_new(FALSE, FALSE, sizeof(unsigned));
    unsigned pageStart = src_cluster.start & 0xfffff000;
    int page_no = 0, codePageNo = 0, count = 0, same_count = 0;
 
    for (; pageStart <= src_cluster.end; pageStart += 0x1000) {
        if (!isPageExist(pageStart, mem1)
            || !isPageExist(pageStart, mem2))
            continue;

        unsigned vAddr = pageStart;
        unsigned pAddr1 =
            vtop(mem1->mem, mem1->mem_size, mem1->pgd, vAddr);
        char *page = (char *) ((unsigned) mem1->mem + pAddr1);

		//print page number
        page_no++;
        //        printf("page %d start vaddr 0x%x\n", page_no, vAddr);
		
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

           if (isKernelAddr(value1, mem1)
               && isKernelAddr(value2, mem2)) {
             if (value1 >= target_cluster.start
                 && value1 <= target_cluster.end
                 && value2 >= target_cluster.start
                 && value2 <= target_cluster.end) {
               int same = (value1 == value2);
               if (same == 1) {
                 same_count++;
               }


               g_array_append_val(pointers, value1);               
               g_array_append_val(pointers2, value2);
               
               //printf("%x,%x/%x %d\n", virAddr, value1, value2, same);
               count++;
             }
           }
        }
    }

    
    printf("pointer number is %d, same no. %d\n", count, same_count);

    //    compute_size(pointers2, mem2);
    return pointers;
}

int print_point_sharp(int level,unsigned vaddr1,Mem * mem1, cluster target){
  int i ;
  if(level > 4) return 1;
  int max_match =10,  match_count =0;
  int offsets[max_match];
  unsigned pointers[max_match];
  int offset1 =0, offset2=0;
  int pre_offset = 0;
  for(i=0; i < PAGE_SIZE; i += 4 ){
    unsigned vAddr1 = vaddr1 +i;
      unsigned pAddr1 =
        vtop(mem1->mem, mem1->mem_size, mem1->pgd, vAddr1);
      unsigned value1 =
        *((unsigned *) ((unsigned) mem1->mem + pAddr1));

      int is_value1_pointer =0;
      if(isKernelAddr(value1, mem1)
         && value1 >= target.start && value1 <= target.end)
        is_value1_pointer = 1;
      
      //if both are heap pointers, to next level
      if( is_value1_pointer ){
        /*
        if(i < 60 && i > 36) continue;

        if(match_count == 0 && i !=4)
          return 0;

        if(match_count == 1 && i !=36)
          return 0;

        if(match_count == 2 && i != 60)
          return 0;
        */

        //filter some sharp
        if( match_count ==0 && i > 500)
          return 0;
        offsets[match_count] = i - pre_offset;
        pre_offset =i;
        pointers[match_count] = value1;
        match_count ++;
      }

      if(match_count >= max_match)
        break;
  }
  if(match_count >= max_match){
    for(i =0;i<max_match; i ++){
      //      printf("%d %x ", offsets[i], pointers[i]);
      printf("%d ", offsets[i]);
    }
    return 1;
  }
  return 0;
}


int contains(GArray * pointers, unsigned value){
     int k;
     int contain = 0;
     for(k = 0; k < pointers->len;k++){
       if (g_array_index(pointers, unsigned, k)==value ){
         contain =1;
         break;
       }
     }
     return contain;
}
GArray *search_obj_baseon_pointer_sharp(cluster src_cluster, cluster target_cluster,
                          Mem * mem1, Mem * mem2)
{
    GArray *pointers = g_array_new(FALSE, FALSE, sizeof(unsigned));
    GArray *pointers2 = g_array_new(FALSE, FALSE, sizeof(unsigned));
    unsigned pageStart = src_cluster.start & 0xfffff000;
    int page_no = 0, codePageNo = 0, count = 0, same_count = 0;
 
    for (; pageStart <= src_cluster.end; pageStart += 0x1000) {
        if (!isPageExist(pageStart, mem1)
            || !isPageExist(pageStart, mem2))
            continue;

        unsigned vAddr = pageStart;
        unsigned pAddr1 =
            vtop(mem1->mem, mem1->mem_size, mem1->pgd, vAddr);
        char *page = (char *) ((unsigned) mem1->mem + pAddr1);

		//print page number
        page_no++;
        //        printf("page %d start vaddr 0x%x\n", page_no, vAddr);
		
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

           //is kernel address
             //is in target area
      
           int isfound = contains(pointers,value1);
             if ( !isfound && isKernelAddr(value1, mem1)
                 && value1 >= target_cluster.start
                 && value1 <= target_cluster.end){
               
               int ret = print_point_sharp(1,value1,mem1, target_cluster);
               if(ret == 1){
                 //                 printf("%x %x\n", virAddr,
                 //                 value1);
                 printf("%x value1\n", value1);
                 g_array_append_val(pointers, value1);
               }
             }

             isfound = contains(pointers2,value2);
             if( !isfound && value1 != value2 && isKernelAddr(value2, mem2)
                 && value2 >= target_cluster.start
                 && value2 <= target_cluster.end) {
               int ret = print_point_sharp(1,value2,mem2, target_cluster);
               if(ret == 1){
                 printf("%x value2\n", value2);
                 g_array_append_val(pointers2, value2);                 
               }
             }
        }
    }  
    compute_size(pointers, mem1);
    compute_size(pointers2, mem2);
    
    return pointers;
}

//print all pointer in some area, offset and value of pointer
GArray *get_reliable_addresses(cluster src_cluster, cluster target_cluster,
                         Mem * mem1, Mem * mem2)
{
    GArray *pointers = g_array_new(FALSE, FALSE, sizeof(unsigned));
    unsigned pageStart = src_cluster.start;
    int page_no = 0, codePageNo = 0;
 
    for (; pageStart <= src_cluster.end; pageStart += 0x1000) {
        if (!isPageExist(pageStart, mem1)
            || !isPageExist(pageStart, mem2))
            continue;

        unsigned vAddr = pageStart;
        unsigned pAddr1 =
            vtop(mem1->mem, mem1->mem_size, mem1->pgd, vAddr);
        char *page = (char *) ((unsigned) mem1->mem + pAddr1);

		//print page number
        page_no++;
        printf("page %d start vaddr 0x%x\n", page_no, vAddr);
		
        //to print all reference to global data area
		//---------------begin--------------
 		int totalPageNumber = mem1->mem_size / PAGE_SIZE;   //assume that every page has 4k
        int calledPages[totalPageNumber];
        int dsmPages[totalPageNumber];
        //record virtual address
        unsigned virtualAddrs[totalPageNumber];
		int i;
        for (i = 0; i < totalPageNumber; i++) {
            calledPages[i] = 0;
            dsmPages[i] = 0;
            virtualAddrs[i] = 0;
        }
        //       if(page_no == 584)
        code_init(mem1, vAddr, PAGE_SIZE, dsmPages, virtualAddrs, 0,
                  calledPages, &codePageNo,pointers,target_cluster);
		//----------------end--------------------		
		
		//        disassemble_getmem_addr(mem1, page, vAddr, target_cluster,
        //                pointers, page_no);
    }
    return pointers;
}

//find the readonly area
cluster findreadonlyarea(Mem * mem, Mem * mem2)
{
    cluster max_cluster;
    cluster src_cluster;
    src_cluster.start = 0;
    src_cluster.end = 0;

    int max_page_count = 0;
    unsigned pageStart = 0x80000000;
    //          unsigned pageStart = 0xc0000000;
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

            if (readonly == 0) {
                int page_count =
                    (src_cluster.end - src_cluster.start) / 0x1000;
                if (page_count > max_page_count) {
                    max_cluster.start = src_cluster.start;
                    max_cluster.end = src_cluster.end;
                    max_page_count = page_count;
                }
                printf("start %x end %x size is %d\n", src_cluster.start,
                       src_cluster.end, page_count);
                src_cluster.start = pageStart;
                readonly = 1;
            }
            //                      printf("%x readonly\n", pageStart);
        } else {
            // printf("%x writable readonly %d\n", pageStart,readonly);
            if (readonly == 1) {
                src_cluster.end = pageStart - 0x1000;
                readonly = 0;
                //   break;
            }
        }
    }
    int page_count = (src_cluster.end - src_cluster.start) / 0x1000;
    if (page_count > max_page_count) {
        max_cluster.start = src_cluster.start;
        max_cluster.end = src_cluster.end;
        max_page_count = page_count;
    }


    printf("max read only area start %x, end %x, size: %d pages\n",
           max_cluster.start, max_cluster.end,
           (max_cluster.end - max_cluster.start) / 0x1000);
    return max_cluster;
}



int isGlobalSystem(unsigned vAddr, Mem * mem)
{
    int rw = 0;                 //read or write
    int us = 0;                 //use or system
    int g = 0;                  //global, no move out of TLB
    int ps = 0;                 //page size 4M or 4k
    unsigned pAddr = vtopPageProperty(mem->mem, mem->mem_size, mem->pgd,
                                      vAddr,
                                      &rw,
                                      &us, &g,
                                      &ps);

    //if the page is system, global
    if (pAddr >= 0 && pAddr < mem->mem_size && us == 0 && g == 256) {
        return 0;
    }
    return -1;
}

void compute_print_size(GArray *data_structs){
   unsigned base_addr = g_array_index(data_structs, unsigned, 0);
    unsigned pre_offset = 0;
    int i;
    for (i = 1; i < data_structs->len; i++) {
        unsigned pre_addr = g_array_index(data_structs, unsigned, i - 1);
        unsigned curr_addr = g_array_index(data_structs, unsigned, i);
        unsigned offset = curr_addr - base_addr;
        int size = offset - pre_offset;
        //     printf("%x %x %d\n", pre_addr, pre_offset, size);
        //        print_type_sequence(pre_addr, curr_addr, mem1);
         pre_offset = offset;
     }
     printf("number of data structure %d\n", data_structs->len);
  }

 GArray *compute_size(GArray * pointers, Mem * mem)
 {
     //sort the pointers
     int compare_unsigned(gpointer a, gpointer b) {
         unsigned *x = (unsigned *) a;
         unsigned *y = (unsigned *) b;
         return *x - *y;
     }

     g_array_sort(pointers, (GCompareFunc) compare_unsigned);

     //remove redundent  and remove not global and system
     GArray *data_structs = g_array_new(FALSE, FALSE, sizeof(unsigned));
     int i;
     unsigned pre_pointer = g_array_index(pointers, unsigned, 0);
     g_array_append_val(data_structs, pre_pointer);
     for (i = 1; i < pointers->len; i++) {
         unsigned curr_pointer = g_array_index(pointers, unsigned, i);
         int gsRet = isGlobalSystem(curr_pointer, mem);
         if (curr_pointer != pre_pointer && gsRet == 0) {
             g_array_append_val(data_structs, curr_pointer);
             pre_pointer = curr_pointer;
         }
     }

     //compute size of data structure
     compute_print_size(data_structs);

     return data_structs;
 }

 unsigned find_data_struct(GArray * data_structs, unsigned target_addr)
 {
     int i = 0;
     for (i = 0; i < data_structs->len - 1; i++) {
         unsigned data_structs_addr =
             g_array_index(data_structs, unsigned, i);

         unsigned next_addr = g_array_index(data_structs, unsigned, i + 1);

         if (data_structs_addr < target_addr && next_addr > target_addr) {
             //      printf("(%d) = *x - %d\n",i,  target_addr- data_structs_addr);
             printf("() = *x - %d\n", target_addr - data_structs_addr);
             break;
         }
         if (data_structs_addr == target_addr) {
             // printf("(%d) = *x\n",i);
             printf("() = *x\n");
             break;

         }
     }
 }



 void print_graph(cluster src_cluster, cluster target_cluster,
                  Mem * mem1, Mem * mem2, GArray * data_structs)
 {
     // GArray *pointers = g_array_new(FALSE, FALSE, sizeof(unsigned));
     unsigned pageStart = src_cluster.start;
     int count = 0, same_count = 0;
     // prinf("src_cluster.end %x\n",src_cluster.end);

     int struct_index = 0;
     int len = data_structs->len;

     unsigned base_addr = g_array_index(data_structs, unsigned, 0);
     unsigned pre_offset = 0;

     unsigned curr_struct_addr = 0;
     for (; pageStart <= src_cluster.end; pageStart += 0x1000) {
         if (!isPageExist(pageStart, mem1)
             || !isPageExist(pageStart, mem2))
             continue;

         unsigned vAddr = pageStart;
         int j;

         for (j = 0; j < 4 * 1024; j += 4) {
             unsigned virAddr = vAddr + j;
             unsigned curr_struct =
                 g_array_index(data_structs, unsigned, struct_index);

             unsigned offset = curr_struct - base_addr;
             unsigned next_struct = 0;
             if (struct_index + 1 < len)
                 next_struct =
                     g_array_index(data_structs, unsigned,
                                   struct_index + 1);
             else
                 next_struct = target_cluster.end;
             int size = next_struct - curr_struct;

             if (virAddr >= curr_struct) {
                 // printf("%d %x %x %d\n", struct_index, curr_struct, offset,
                 //           size);
                 curr_struct_addr = curr_struct;
                 printf("%x %d\n", offset, size);
                 struct_index++;

                 if (struct_index >= len)
                     break;
             }
             //print pointers
             unsigned pAddr1 =
                 vtop(mem1->mem, mem1->mem_size, mem1->pgd, virAddr);
             unsigned value1 =
                 *((unsigned *) ((unsigned) mem1->mem + pAddr1));

             unsigned pAddr2 =
                 vtop(mem2->mem, mem2->mem_size, mem2->pgd, virAddr);
             unsigned value2 =
                 *((unsigned *) ((unsigned) mem2->mem + pAddr2));

             if (isKernelAddr(value1, mem1)
                 && isKernelAddr(value2, mem2)) {
                 if (value1 >= target_cluster.start
                     && value1 <= target_cluster.end
                     && value2 >= target_cluster.start
                     && value2 <= target_cluster.end) {
                     int same = (value1 == value2);
                     if (same == 1) {
                         same_count++;
                         //                        printf("%x --> %x\n", virAddr, value1);
                         int inter_offset = virAddr - curr_struct_addr;
                         unsigned abs_offset = virAddr - base_addr;
                         printf("pointer: %d %x\n", inter_offset,
                                abs_offset);
                         /*
                            int offset = value1 - virAddr;
                            if (offset > 4096 || offset < -4096)
                            find_data_struct(data_structs, value1);
                            else {

                            if (offset > 0)
                            printf("x= *x - %d\n", offset);
                            if (offset == 0)
                            printf("x= *x\n");
                            if (offset < 0)
                            printf("x= *x + %d\n", -offset);

                            }
                          */
                     }

                     count++;
                 }
             }
         }

     }
     printf("pointer number is %d, same no. %d\n", count, same_count);

     return;
 }


 void print_type_sequence(unsigned start_addr, unsigned end_addr,
                          Mem * mem1)
 {
     int isString(unsigned value) {
         int isString = 0;       // default is string
         // >= 33 <= 126 ascii code 
         char byte = value & 0x000f;
         if ((int) byte < 33 || (int) byte > 126)
             return -1;
         if (((value >> 4) & 0x000f) < 33 || ((value >> 4) & 0x000f) > 126)
             return -1;
         if (((value >> 8) & 0x000f) < 33 || ((value >> 8) & 0x000f) > 126)
             return -1;
         if (((value >> 12) & 0x000f) <
             33 || ((value >> 12) & 0x000f) > 126)
             return -1;

         return isString;
     }

     unsigned virtual_address = start_addr;
     while (virtual_address < end_addr) {
         unsigned p_addr =
             vtop(mem1->mem, mem1->mem_size, mem1->pgd, virtual_address);
         unsigned value = *((unsigned *) ((unsigned) mem1->mem + p_addr));
         if (value == 0)
             printf("%c", '0');
         else if (isKernelAddr(value, mem1))
             printf("%c", 'A');
         else if (isString(value) == 0)
             printf("%c", 'S');
         else
             printf("%c", 'D');


         virtual_address += 4;
     }
     puts("");
     return;
 }


 /*determine whether a value is a valid non-empty kernel point */
 int isKernelAddr(unsigned vaddr, Mem * mem)
 {
     //special pointer
     if (vaddr == 0x00100100 || vaddr == 0x00200200)
         return 1;

     //non pointer
     if (vaddr == 0xcccccccc)
         return 0;

     //normal pointer
     unsigned kernleStartAddr;
     kernleStartAddr = 0x80000000;
     if (vaddr > kernleStartAddr) {
         unsigned pAddr = vtop(mem->mem, mem->mem_size, mem->pgd, vaddr);
         if (pAddr > 0 && pAddr < mem->mem_size)
             return 1;
     }
     return 0;
 }

 int isNonPointer(unsigned vir_add, Mem * mem)
 {
     if (vir_add == 0xcccccccc)
         return 1;
     return !isKernelAddressOrZero(vir_add, mem);
 }

 int pointer_match(unsigned vAddr, Mem * mem1, Mem * mem2,
                   int *not_match_no)
 {
     int j;
     int pointsMatch = 0;
     for (j = 0; j < PAGE_SIZE; j += 4) {
         unsigned virAddr = vAddr + j;
         unsigned pAddr1 =
             vtop(mem1->mem, mem1->mem_size, mem1->pgd, virAddr);
         unsigned pAddr2 =
             vtop(mem2->mem, mem2->mem_size, mem2->pgd, virAddr);

         //get value by assume its a point
         unsigned value1 = *((unsigned *) ((unsigned) mem1->mem + pAddr1));
         unsigned value2 = *((unsigned *) ((unsigned) mem2->mem + pAddr2));

         if ((isKernelAddr(value1, mem1)
              && (isKernelAddressOrZero(value2, mem2)))
             || (isKernelAddressOrZero(value1, mem1)
                 && isKernelAddr(value2, mem2))) {
             pointsMatch++;
             if (value1 != 0 && value2 != 0) {
                 (*not_match_no) = 0;    //reset pointNotMatch, since strong match
                 printf("%x\n", virAddr);
                 //printf("%x %x %x %d\n", virAddr, value1, value2,
                 //     value1 == value2);
             }
             continue;
         }
         //value1 not equals to value2
         if (value1 != value2) {
             if ((isKernelAddr(value1, mem1)
                  && isNonPointer(value2, mem2))
                 || (isNonPointer(value1, mem1)
                     && isKernelAddr(value2, mem2))) {
                 (*not_match_no)++;
                 //                              printf("%x %x not pointer\n", value1,value2);
                 if ((*not_match_no) > 800) {
                     printf("end is %x\n", virAddr);
                     return 0;
                 } else
                     continue;
             }
         }
     }

     return 1;
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

 //Get all addresses from source area to target area
 GArray * get_all_addresses_src2target(Mem * mem1, Mem * mem2, cluster src_cluster,cluster targetcluster){
    GArray *pointers =
         get_all_addresses(src_cluster, targetcluster, mem1, mem2);
    puts("------------------sort and print------------------");
    GArray *data_structs = compute_size(pointers, mem1);
   g_array_free(pointers, FALSE);
   return data_structs;
}

//print the pages which have pointers
GArray * has_pointers(Mem * mem1, Mem * mem2, cluster src_cluster,cluster target_cluster){
    GArray *pointers = g_array_new(FALSE, FALSE, sizeof(unsigned));
    unsigned pageStart = src_cluster.start & 0xfffff000;
    int page_no = 0, codePageNo = 0, count = 0, same_count = 0;
    int pre_pointer_no =0;
    for (; pageStart <= src_cluster.end; pageStart += 0x1000) {
        if (!isPageExist(pageStart, mem1)
            || !isPageExist(pageStart, mem2))
            continue;

        unsigned vAddr = pageStart;
        unsigned pAddr1 =
            vtop(mem1->mem, mem1->mem_size, mem1->pgd, vAddr);
        char *page = (char *) ((unsigned) mem1->mem + pAddr1);

		//print page number
        page_no++;
		
        int j;
        int pointer_no_in_page = 0;
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

           if (isKernelAddr(value1, mem1)
               && isKernelAddr(value2, mem2)
               && value1 >= target_cluster.start
               && value1 <= target_cluster.end
               && value2 >= target_cluster.start
               && value2 <= target_cluster.end) {
             pointer_no_in_page ++;
             int same = (value1 == value2);
             if (same == 1) {
               same_count++;
               g_array_append_val(pointers, value1);
             }
             count++;

             if(pageStart ==  0xc04a8000 || pageStart ==  0xc0494000 ||pageStart ==  0xc0495000  || pageStart ==  0xc0497000 )
               printf("%x,%x/%x %d\n", virAddr, value1, value2, same);
           }
        }

        printf("%d 0x%x %d\n", page_no, vAddr, pointer_no_in_page);

        /*
        if((pointer_no_in_page > 0 && pre_pointer_no == 0 )||(pointer_no_in_page == 0 && pre_pointer_no > 0 ) ){
          printf("page %d start vaddr 0x%x\n", page_no, vAddr);
          pre_pointer_no = pointer_no_in_page;
        }
        */
    }

    return pointers;
}


int find_first_pointer_next_level(int level,unsigned vaddr1, unsigned vaddr2,Mem * mem1, Mem * mem2, cluster heap, cluster global_data,cluster readonly){
  int i ;
  if(level > 4) return 1;
  for(i=0; i < PAGE_SIZE; i += 4 ){
    unsigned vAddr1 = vaddr1 +i;
    unsigned vAddr2 = vaddr2 +i;
      unsigned pAddr1 =
        vtop(mem1->mem, mem1->mem_size, mem1->pgd, vAddr1);
      unsigned value1 =
        *((unsigned *) ((unsigned) mem1->mem + pAddr1));

      unsigned pAddr2 =
        vtop(mem2->mem, mem2->mem_size, mem2->pgd, vAddr2);
      unsigned value2 =
        *((unsigned *) ((unsigned) mem2->mem + pAddr2));

      int is_value1_pointer =0;
      if(isKernelAddr(value1, mem1))
        is_value1_pointer = 1;

      int is_value2_pointer =0;
      if ( isKernelAddr(value2, mem2))
        is_value2_pointer = 1;

      //if both are heap pointers, to next level
      if( is_value1_pointer && is_value2_pointer
          && value1 >= heap.start && value1 <= heap.end
          && value2 >= heap.start && value2 <= heap.end){
        int ret =  find_first_pointer_next_level(level+1, value1,value2,mem1,mem2,heap,global_data,readonly);
        if (ret == 1){
          printf("(%d,%d,%x/%x)",level,i,value1,value2);
          return 1;
          }
      }
      
      int is_global_or_readonly(unsigned address,cluster global_data,cluster readonly){
        if((address >=global_data.start && address <= global_data.end )
           || (address >= readonly.start && address <= readonly.end ))
          return 1;
        else
          return 0;
      }
      
      //if one is pointer and the other is zero, match, return 1
      if( ( is_value1_pointer == 1 && is_global_or_readonly(value1,global_data,readonly)
            && (value2 == 0 || value1 == value2))||
          ( value1 == 0 && is_value2_pointer == 1 && is_global_or_readonly(value2,global_data,readonly) )){
        printf("%x and %x pointer vs zero or global and readonly\n",value1,value2);
        return 1;
      }

      if(isWeakKernelAddress(value1,mem1) && isWeakKernelAddress(value2,mem2))
        {
          if(value1 != 0 && value2 != 0)
            printf("%x and %x are weakKernelAddress\n", value1,value2);
          continue;
        }
      
      //if one of value is pointer and the other is not, no match,
      //return 0, loose
      if( ( is_value1_pointer == 1 && is_value2_pointer == 0 )||
          ( is_value1_pointer == 0 && is_value2_pointer == 1 )
          ){
        printf("%x %x no match\n",value1,value2);
        return 0;
      }

  }
  //if in length
  return 1;
}

// 3depth, only see the first pointer of data, print the path
//
GArray * recursive_data_heap(Mem * mem1, Mem * mem2, cluster global_data,cluster heap, cluster readonly){
  GArray *pointers = g_array_new(FALSE, FALSE, sizeof(unsigned));
  //start from beginning of page
  unsigned pageStart = global_data.start & 0xfffff000; 
  int pointer_no =0, reliable_pointer_no =0;
  
  for (; pageStart <= global_data.end; pageStart += 0x1000) {
    if (!isPageExist(pageStart, mem1)
        || !isPageExist(pageStart, mem2))
      continue;

        unsigned vAddr = pageStart;
        unsigned pAddr1 =
            vtop(mem1->mem, mem1->mem_size, mem1->pgd, vAddr);
        char *page = (char *) ((unsigned) mem1->mem + pAddr1);
		
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

           if (isKernelAddr(value1, mem1)
               && isKernelAddr(value2, mem2)
               && value1 >= heap.start
               && value1 <= heap.end
               && value2 >= heap.start
               && value2 <= heap.end) {
             pointer_no ++;
             //check whether pointers is reliable
             int ret =find_first_pointer_next_level(1, value1,value2,mem1,mem2,heap,global_data,readonly);
             if (ret == 1){
               //               printf("\n%x,%x/%x\n", virAddr, value1, value2);
               reliable_pointer_no++;
               g_array_append_val(pointers, virAddr);
             }
             printf("\n%x,%x/%x\n", virAddr, value1, value2);
           }
        }
  }
  printf("pointers no %d, reliable pointer %d\n",pointer_no, reliable_pointer_no);

  int i;
  for(i=0;i<pointers->len;i++)
    {
      unsigned addr = g_array_index(pointers, unsigned, i);
      printf("%x\n",addr);
    }
  return pointers;
}

//Get all addresses from Read only area
GArray* get_reliable_addresses_from_ro(Mem * mem1, Mem * mem2, cluster src_cluster,cluster targetcluster){

  puts("------------------Get reliable addresses from Read only area -----------------");
  GArray *reliable_addrs =
    get_reliable_addresses(src_cluster, targetcluster, mem1, mem2);
  puts("------------------sort points and print them------------------");
  GArray *reliable_data_structs = compute_size(reliable_addrs, mem1);

  g_array_free(reliable_addrs, FALSE);
  return reliable_data_structs;
}

//get pointers2 except pointers1, these are two sorted arrays
GArray * getIntersection( GArray *pointers1,  GArray *pointers2){
  GArray * intersection  = g_array_new(FALSE, FALSE, sizeof(unsigned));  
  int i1, i2;
  for ( i1 =0,i2=0; i1< pointers1 -> len && i2 < pointers2 -> len; ){
    unsigned pointer1  = g_array_index(pointers1, unsigned, i1);
    unsigned pointer2  = g_array_index(pointers2, unsigned, i2);
    if (pointer1 == pointer2) {
      i1++;
      i2++;
      continue;
    }
    if (pointer1 < pointer2){
      i1 ++;
    }else{
      //pointer1 > pointer2
      g_array_append_val(intersection, pointer2);
      printf("%x\n", pointer2);
      i2 ++;
    }
      
  }

  printf("diff len is %d\n", intersection -> len);
  return intersection;
}


void build_graph(Mem * mem1, Mem * mem2)
{

    puts("------------------Get all pointers with recursion------------------");
    cluster src_cluster;
    src_cluster.start = 0xc0000000;
    src_cluster.end= 0xffffe000;

    cluster targetcluster;
    targetcluster.start = 0xc0000000;
    targetcluster.end = 0xffffe000;

    GArray * allpointers=get_all_addresses_src2target(mem1,mem2,src_cluster,targetcluster);
    printf("allpointers len is %d\n", allpointers->len);


    /*
    //all pointers, compare the first match of all pointers,
    puts("------------------tranverse one by one------------------");
    //if first level offset is same and the next level matches, than keep this pointer and add to same
    //class
    GHashTable* class_hash = g_hash_table_new(g_str_hash, g_direct_equal);
    int i ;
    for (i=0;i< allpointers -> len; i ++){
      unsigned pointer =g_array_index(allpointers, unsigned, i);
      int ret = print_point_sharp(1,pointer,mem2, targetcluster);
      if(ret == 1){
        printf("%x \n", pointer);
        g_array_append_val(pointers, pointer);
      }
    }
    
    g_hash_table_insert(hash, "4", allpointers);
    g_hash_table_insert(hash, "12", allpointers);
    //    printf("There are %d keys in the hash\n", g_hash_table_size(hash));
    allpointers = (GArray *)g_hash_table_lookup(hash, "43");
    printf("The 4 len is %x\n", allpointers);
    if (allpointers != 0)
      printf("The 4 len is %d\n", allpointers->len);
    return;
    GArray *pointers = g_array_new(FALSE, FALSE, sizeof(unsigned));
    */
    return;
}

//given two mem which one have more processes than other, get the
//different parts
void intersect_mems(Mem * mem1, Mem * mem2)
{
    cluster src_cluster;
    src_cluster.start = 0xc0000000;
    src_cluster.end= 0xffffe000;

    cluster targetcluster;
    targetcluster.start = 0xc0000000;
    targetcluster.end = 0xffffe000;

    GArray * allpointers1=get_all_addresses_src2target(mem1,mem2,src_cluster,targetcluster);
    GArray * allpointers2=get_all_addresses_src2target(mem2,mem1,src_cluster,targetcluster);
    //get intersection
    GArray * allpointers = getIntersection(allpointers1, allpointers2);
    puts("------------------tranverse one by one------------------");
    GArray *pointers = g_array_new(FALSE, FALSE, sizeof(unsigned));
    int i ;
    for (i=0;i< allpointers -> len; i ++){
      unsigned pointer =g_array_index(allpointers, unsigned, i);
      int ret = print_point_sharp(1,pointer,mem2, targetcluster);
      if(ret == 1){
        printf("%x \n", pointer);
        g_array_append_val(pointers, pointer);
      }
    }
    return;
}

void find_global_areas(Mem * mem1, Mem * mem2){
    puts("-----------------find text(read only) area-----------------");
    cluster readonly_cluster = findreadonlyarea(mem1, mem2);

    puts("-----------------find global data area-----------------");
    cluster targetcluster;
    targetcluster.start = readonly_cluster.end + 0x1000;
    //    targetcluster.end = findLoop(readonly_cluster, mem1, mem2);
    targetcluster.end = 0xffffe000;

    //       puts("------------------Get all addresses from Read only area ------------------");
    //    get_all_addresses_src2target(mem1,mem2,readonly_cluster,targetcluster);

    //get global data area by reliable disassemble
    GArray *reliable_data_structs=  get_reliable_addresses_from_ro(mem1,mem2,readonly_cluster,targetcluster);
    cluster global_data_cluster;
    global_data_cluster.start = g_array_index(reliable_data_structs, unsigned, 0);
    global_data_cluster.end = g_array_index(reliable_data_structs, unsigned, reliable_data_structs->len -1);
    printf("global data start %x, end %x \n",global_data_cluster.start, global_data_cluster.end);
    g_array_free(reliable_data_structs, FALSE);


    //throw the areas which have no pointers to heap
    //targetcluster.start = (global_data_cluster.end & 0xfffff000) +  0x1000;
    //    targetcluster.end = 0xffffe000;

    //throw the areas which have no pointers to global data
    //    targetcluster.start = global_data_cluster.start & 0xfffff000;
    //    targetcluster.end = (global_data_cluster.end & 0xfffff000) +  0x1000;

    //    GArray * pointers = has_pointers(mem1,mem2,global_data_cluster,targetcluster);
    //    g_array_free(pointers, FALSE);

    /*
    puts("------------------Get all addresses from global data area to ro area------------------");
    targetcluster.start = 0xc0000000;
    targetcluster.end = global_data_cluster.start & 0xfffff000;
    GArray * data2ro = get_all_addresses_src2target(mem1,mem2,global_data_cluster,targetcluster);


    punts("------------------Get all addresses from global data area to global data area------------------");
    targetcluster.start = global_data_cluster.start & 0xfffff000;
    targetcluster.end = (global_data_cluster.end & 0xfffff000) +  0x1000;
    GArray * data2data=get_all_addresses_src2target(mem1,mem2,global_data_cluster,targetcluster);

    */

   
    puts("------------------Get all addresses from global data area to other areas(heap)------------------");
    targetcluster.start = (global_data_cluster.end & 0xfffff000) +  0x1000;
    targetcluster.end = 0xffffe000;
    GArray * data2heap=get_all_addresses_src2target(mem1,mem2,global_data_cluster,targetcluster);

    puts("------------------recursive data in heap------------------");
    cluster heap_area;
    heap_area.start = g_array_index(data2heap, unsigned, 0);
    heap_area.end = g_array_index(data2heap, unsigned, data2heap->len -1);
    //    GArray * heap2heap=get_all_addresses_src2target(mem1,mem2,heap_area,targetcluster);
    recursive_data_heap(mem1,mem2,global_data_cluster,heap_area,readonly_cluster);

    
    /*
    int total = data2ro->len + data2data->len +data2heap->len;
    printf ("data2ro %d, data2data %d, data2heap %d\n",data2ro->len, data2data->len, data2heap->len);
    printf ("data2ro  %f, data2data %f, data2heap %f\n", (float)data2ro->len/total,(float)data2data->len/total, (float)data2heap->len/total);
    g_array_free(data2ro, FALSE);
    g_array_free(data2data, FALSE);
    */
    g_array_free(data2heap, FALSE);

    return;
}
