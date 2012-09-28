#! /usr/bin/env python
import sys
import os


def getPotentialPgd(pageNo, pageSize,memFile ):
    pgdcount = 0;
    for i in range(0, pageNo):
        startAddr = i * pageSize;
        isPgd = 1;
        
        #scan low 2k, 0 - 0x7ff
        for j in range(0, 2 * 1024,4):
            memFile.seek(startAddr + j)
            buf =memFile.read(4)
            if ((pdeItem & 0x418) != 0):
                isPgd = 0;
                break;

        if (isPgd == 0):
            continue;

        #scan high 2k, from 0x800
        matchCount = 0;
        for j in range(2 * 1024, 4 * 1024, 4) :
            unsigned pdeItem = *(unsigned *) ((unsigned) mem + startAddr + j);
            if (pdeItem == 0 || (pdeItem & ~0xfff) > mem_size):
                continue;

            if ((pdeItem & 0x418) != 0):break;

            if ((pdeItem & 0x3) == 0x3 && (pdeItem & 0x20) == 0x20 && (pdeItem & 0x418) == 0):
                #check pte
                if isPteValid(pdeItem & ~0xfff, mem, mem_size):
                    matchCount = matchCount + 1

            #detemine whether this page is a pgd
            if (isPgd == 1 && matchCount >= 1):
                pageIndex[i] = 1;
                pgdcount = pgdcount +1 

    printf("potential pdg count: %d\n", pgdcount);
    return pgdcount;

def getPgd(filename):
    pgd =0;
    pageSize = 4096
    pageNo = os.path.getsize(filename)/pageSize
    print pageNo
    f = open(filename,'rb')
    getPotentialPgd(pageNo, pageSize, f )
    return pgd

if __name__ == "__main__":
    filename = sys.argv[1]
    f = open(filename,'rb')
    
    pageCount =0
    while True:
        buf = f.read(4096) # byte number
        if len(buf) == 0:
            break
        else:
            pageCount = pageCount +1

    print "page count is %d" % pageCount 
