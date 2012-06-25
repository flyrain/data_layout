import sys
filename = sys.argv[1]
#writable_begin_addr = sys.argv[2]
f = open(filename,'r')
line = f.readline()
count =0;
addr1 = ''
addr2 = ''
pairs =[]
while(line != ''):
    if line[5:8] == '000' and ( not line.__contains__('*')) and (not line.__contains__('/')) :
       # print line
        array=line.replace(':',' ').split()
        tmp_addr1 = array[0]
        tmp_addr2 = array[3]
        if (not (tmp_addr1 == addr1 and tmp_addr2 == addr2)) and (not ((tmp_addr1 == addr2 and tmp_addr2 == addr1))) :
            addr1 = tmp_addr1
            addr2 = tmp_addr2
            pair =[]
            pair = pair +[addr1,addr2]
            count_pair =[]
            count_pair = count_pair + [addr2,addr1]
            if(not pairs.__contains__(pair) and not pairs.__contains__(count_pair)):
                pairs.append(pair)
                print pair 
                count = count +1
    
    line = f.readline()


print 'pairs count is ',
print count


end_readonly_addr = int(pairs[len(pairs)-1][0],16)
data_start_addr = end_readonly_addr +4096

"sort by second column in pairs"
sorted_pair =sorted(pairs, key=lambda student: student[1])

for pair in sorted_pair:
    if int(pair[1],16) >= data_start_addr:
        print pair[1]


readonly_size =(0xc042c000-0xc0100000)*1.5



data_end_addr = data_start_addr

for pair in pairs:
    if int(pair[0],16) >= end_readonly_addr:
        break
    curr_addr = int(pair[1],16)
    if curr_addr - data_end_addr >= readonly_size:
        continue
 # if curr_addr >= 0xd0000000L or curr_addr >= 0xcc000000L or  curr_addr >= 0xc1660000L  :
   #     continue
    if curr_addr >= data_end_addr:
        data_end_addr = curr_addr
        print hex(data_end_addr)

print 'golobal data begin ',
print hex(data_start_addr)
print 'golobal data end ',
print hex(data_end_addr)
