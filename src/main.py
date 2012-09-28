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

    print "potential pdg count: %d" % len(potentialPgdIndx)
    return potentialPgdIndx;


def isEqual(char *mem, unsigned paddr1, unsigned paddr2, int length) :
    '''
    if two pgd page is equal
    '''
    noMatchCount = 0;
    unsigned value1s[512];
    unsigned value2s[512];
    int indexs[512];
    for (i = 0; i < length; i = i + 4):
        unsigned value1 = *(unsigned *) ((unsigned) mem + paddr1 + i);
        unsigned value2 = *(unsigned *) ((unsigned) mem + paddr2 + i);
        if (value1 != value2):
            value1s[noMatchCount] = value1;
            value2s[noMatchCount] = value2;
            indexs[noMatchCount] = i;
            noMatchCount++;

    float matchRate = (float) noMatchCount / (float) (length / 4)
    #rate less than 0.02
    if (matchRate <= 0.02):
        return True
    else:
        return False



def  getPgdReal(potentialPgds, pageSize, char *mem):
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
            if isEqual(mem, paddr1, paddr2, (4 - startIndex) * 1024):
                matchNumber[i]++;
                countedpages[j] = i;

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

    print "Real PGD Number is %d, PGD physical address:0x%x\n" % (maxMatch + 1, pgdPhyAddr)
    return pgdPhyAddr;


def getPgd(filename):
    '''
    find the pgd of memory
    '''
    pageSize = 4096
    memSize = os.path.getsize(filename)
    pageNo = memSize/pageSize
    print memSize,pageNo, pageSize
    f = open(filename,'rb')
    memData = f.read()
    potentialPgdIndx = getPotentialPgd(pageNo, pageSize, memData)
    #print potentialPgdIndx
    getRealPgd(memData, potentialPgdIndx, pageSize)
    pgd = 0
    return pgd

if __name__ == "__main__":
    filename = sys.argv[1]
    getPgd(filename)
