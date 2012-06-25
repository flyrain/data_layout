import sys
filename1 = sys.argv[1]
filename2 = sys.argv[2]



def get_addrs(filename):
    f = open(filename,'r')

    lines = f.readlines()
    addrs =[]
    for line in lines:
        line_array = line.split()
        if len(line_array) == 4 and (line_array[3]=='1' or line_array[3]=='0') :
            addrs.append(line_array[0])
      
    return addrs;


addrs1 = get_addrs(filename1)
addrs2 = get_addrs(filename2)

comm_addr =[]
for addr in addrs1:
    if addrs2.__contains__(addr):
        comm_addr.append(addr)


print comm_addr
print len(addrs1),
print len(addrs2),
print len(comm_addr)

