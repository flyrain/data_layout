import sys
filename = sys.argv[1]
f = open(filename,'r')
line = f.readline()

addresses= []

while(line != ''):
    if line[26:28] == ' 1':
        array=line.replace('/',' ').replace(',', ' ').split()
        addr = array[1]
        if  int(addr,16)%2 == 0:
            isfind =0
            for i in range(len(addresses)):
                if addresses[i][0] == addr:
                    addresses[i][1] = addresses[i][1]+1
                    isfind =1
                    break;
            
            if isfind == 0:
                addresses.append([addr,1])
#            if (not  addresses.__contains__(addr)) :
 #               addresses.append(addr)
                #print addr
            
    line = f.readline()


for item in addresses:
    print item[0],
    print item[1]

print len(addresses)
