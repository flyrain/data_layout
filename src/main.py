#! /usr/bin/env python
import sys
import os
import struct

def isPteValid( pteStart, memData):
    isValid = True;
    matchCount = 0;
    for k in range(0, 4 * 1024, 4):
        addr = pteStart + k
        pte =  struct.unpack('I', memData[addr: (addr+4)])[0]
        #printf("pte:%x\n",pte);
        if ((pte & 0x1) == 0x1 and (pte & ~0xfff) > len(memData)):
            continue;

        if (pte == 0):
            continue

        us = pte & (1 << 2);
        g = pte & (1 << 8);

        if ((pte & 0x1) == 0x1 and us == 0 and g == 256) :
            matchCount = matchCount + 1
	else:
            isValid = False;
            break;
	

    if (matchCount == 0):
        isValid = False;

    return isValid;


def getPotentialPgd(pageNo, pageSize, memData):
    potentialPgdIndx = []
    for i in range(0, pageNo):
        startAddr = i * pageSize;
        isPgd = 1;
        
        #scan low 2k, 0x0 - 0x7ff
        for j in range(0, 2 * 1024,4):
            addr = startAddr +j
            pdeItem=  struct.unpack('I', memData[addr: (addr+4)])[0]
            if (pdeItem & 0x418) != 0:
                isPgd = 0;
                break;

        if (isPgd == 0):
            continue;

        #scan high 2k, from 0x800
        matchCount = 0;
        for j in range(2 * 1024, 4 * 1024, 4) :
            addr = startAddr +j
            pdeItem=  struct.unpack('I', memData[addr: (addr+4)])[0]
            if (pdeItem == 0 or (pdeItem & ~0xfff) > len(memData)):
                continue;
            if ((pdeItem & 0x418) != 0):break;
            if ((pdeItem & 0x3) == 0x3 and (pdeItem & 0x20) == 0x20 and (pdeItem & 0x418) == 0):
                #check pte
                if isPteValid(pdeItem & ~0xfff, memData):
                    matchCount = matchCount + 1

        #detemine whether this page is a pgd
        if (isPgd == 1 and matchCount >= 1):
            potentialPgdIndx.append(i)

    print "Potential PDG number: %d" % len(potentialPgdIndx)
    return potentialPgdIndx;


def isEqual(memData, paddr1, paddr2, length) :
    '''
    if two pgd page is equal
    '''
    noMatchCount = 0.0
    for i in range (0, length,4):
        addr = paddr1 + i
        value1 =  struct.unpack('I', memData[addr: (addr+4)])[0]
        addr = paddr2 + i
        value2 = struct.unpack('I', memData[addr: (addr+4)])[0]
        if (value1 != value2):
            noMatchCount = noMatchCount + 1

    matchRate =  noMatchCount /  (length / 4)

    #rate less than 0.02
    if (matchRate <= 0.02):
        return True
    else:
        return False


def getRealPgd(potentialPgds, pageSize, memData):
    '''
    get real pgd page by compare all field of 3*1024 to 4*1024
    '''
    #array to record match number,initiate all to 0
    matchNumber = []
    #array to record the page which have been count as one of the same pages
    #if i is handled, set countedpages[i] by the first same index, else countedpages[i] is -1
    countedpages =[]
    for i in range(len(potentialPgds)):
        matchNumber.append(0)
        countedpages.append(-1)

    #find the max match, and the physical address of pgd
    for i in range(len(potentialPgds)):
        #if page i is handled, do nothing
        if (countedpages[i] >= 0):
            continue;

        #compare i and i+1,i+2...len(potentialPgds)-1
        for j in range (i + 1, len(potentialPgds)):
            #if page j is handled, do nothing
            if (countedpages[j] >= 0): continue
            #compare i and j
            startIndex = 2;
            paddr1 = potentialPgds[i] * pageSize + startIndex * 1024;
            paddr2 = potentialPgds[j] * pageSize + startIndex * 1024;
            if isEqual(memData, paddr1, paddr2, (4 - startIndex) * 1024):
                matchNumber[i] = matchNumber[i] +1
                countedpages[j] = i

    #if current match number is bigger than max Match number,
    #then update max Match number, and record the phycial address
    maxMatch = 0;
    maxIndex = 0;
    for i in range(len(potentialPgds)):
        if (matchNumber[i] > maxMatch) :
            maxMatch = matchNumber[i];
            maxIndex = i;

    pgdPhyAddr = potentialPgds[maxIndex] * pageSize;

    #printf all real pgds
    print "Real PGD Number is %d, PGD physical address:0x%x" % (maxMatch + 1, pgdPhyAddr)
    return pgdPhyAddr


def getPgd(memData, pageSize):
    '''
    find the pgd of memory
    '''
    potentialPgdIndx = getPotentialPgd(pageNo, pageSize, memData)
    #print potentialPgdIndx
    pgd =  getRealPgd( potentialPgdIndx, pageSize,memData)
    return pgd


def vtop(memData,  pgd,  addr):
    '''
    virtual address to physical address
    '''
    size = len(memData)
    #get pde
    pde_addr = (pgd & ~0xfff) + ((addr >> 20) & ~3);
    if (pde_addr > size):
        print "ERROR 1 addr %x pde %x\n" % (addr, pde_addr)
        return -1;

    pde = struct.unpack('I', memData[pde_addr: (pde_addr+4)])[0]
	
    #get pte
    pte = 0
    pte_addr = 0
    page_size = 0 

    PG_PRESENT_BIT = 0
    PG_PRESENT_MASK = 1 << PG_PRESENT_BIT
    if (pde & PG_PRESENT_MASK) == 0 : return -1;

    PG_PSE_BIT = 7
    PG_PSE_MASK = (1 << PG_PSE_BIT)
    if (pde & PG_PSE_MASK):
        pte = pde & ~0x003ff000;
        page_size = 4096 * 1024;
    else:
        pte_addr = ((pde & ~0xfff) + ((addr >> 10) & 0xffc));
        if (pte_addr > size): return -1
        pte = struct.unpack('I', memData[pte_addr: (pte_addr+4)])[0]
        if (pte & PG_PRESENT_MASK) == 0: 	return -1
        page_size = 4096;

    #get paddr
    page_offset = addr & (page_size - 1);
    
    TARGET_PAGE_SIZE = (1 << 12)
    TARGET_PAGE_MASK = ~(TARGET_PAGE_SIZE -1)
    
    paddr = (pte & TARGET_PAGE_MASK) + page_offset;

    if (paddr >= size): return -1;

    return paddr;



def isKernelAddr(vaddr, memData, pgd):
    '''
    determine whether a value is a valid non-empty kernel point
    '''
     #special pointer
    if (vaddr == 0x00100100 or vaddr == 0x00200200):
        return True

     #non pointer
    if (vaddr == 0xcccccccc):
        return False;

     #normal pointer
    if (vaddr > 0xc0000000):
        pAddr = vtop(memData, pgd, vaddr)
        if (pAddr >= 0 and pAddr <= len(memData)):
            return True

    return False;


def getAddressSrc2tar(src, target, memData, pgd):
    '''
    Get all pointers from src to target
    '''
    addresses =[]
    beginAddr = src['start'] & 0xfffff000;
 
    for pageStart in range(beginAddr, src['end'], 0x1000):
        #find page start physical address, then interate the whole page
        pAddr = vtop(memData, pgd, pageStart)
        if pAddr < 0 or pAddr > len(memData): continue
        for i in range(0,  4 * 1024, 4):
            addr = pAddr + i 
            value = struct.unpack('I', memData[addr: (addr+4)])[0]
           
            if isKernelAddr(value, memData, pgd) and value >= target['start'] and value < target['end']:
                print hex(value)
                addresses.append(value)
    
    print "Pointer Number: %d" % len(addresses) 

   #    compute_size(pointers2, mem2);
    return addresses

def findClass(memData, pageSize, pgd):
    src = {"start":0xc0000000, "end":0xffffe000}
    target = {"start":0xc0000000, "end":0xffffe000}
    getAddressSrc2tar(src,target, memData, pgd)

if __name__ == "__main__":
    filename = sys.argv[1]
    pageSize = 4096
    memSize = os.path.getsize(filename)
    pageNo = memSize/pageSize
    print "Mem Size %d Page Number %d Page Size %d" % (memSize,pageNo, pageSize)
    f = open(filename,'rb')
    memData = f.read()
    pgd = getPgd(memData, pageSize)
    findClass(memData, pageSize,pgd)

