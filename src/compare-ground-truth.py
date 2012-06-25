import sys
#read all addr
filename = sys.argv[1]
f1 = open(filename,'r')

lines = f1.readlines()
addresses =[]
for line in lines:
    addresses.append(line.strip())
    
#print addresses

#system map
system_map_filename = sys.argv[2]
sys_map = open(system_map_filename, 'r')
sys_map_line = sys_map.readline()

got_count=0
total =0
while(sys_map_line  != ''):
    linearray =sys_map_line.split()
    if linearray[1].upper() != 'D' and  linearray[1].upper() != 'B':
        sys_map_line = sys_map.readline()
        continue

    total = total + 1
    addr= linearray[0]
    if addresses.__contains__(addr):
        got_count = got_count +1
#        print sys_map_line,
        print addr

    sys_map_line = sys_map.readline()


print "got {0} in {1}. pointer {2}".format( got_count, total, len(lines)-1)
