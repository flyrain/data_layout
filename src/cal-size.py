import sys
filename = sys.argv[1]
f = open(filename,'r')

lines = f.readlines()
base = int(lines[1].strip(),16)
curr_addr = base
print lines[1].strip(),
print 0,
print 0
for line in lines[2:]:
    addr = int(line.strip(),16)
    if(addr%2 != 0): #odd number is rubbish
        continue

    offset = addr - base
    size = addr - curr_addr
    curr_addr = addr

    print line.strip(),
    print offset,
    print size
