import sys
filename1 = sys.argv[1]
#filename2 = sys.argv[2]



def get_sigs(filename):
    f = open(filename,'r')
    line = f.readline()

    sigs= []

    begin =0
    while(line != ''):
        if line.__contains__('same no.'):
            begin =1
            line = f.readline()
            continue
        
        if line.__contains__('number of data structure'):
            begin =0

        if begin == 1:
            offset =line.split()[1]
            size = line.split()[2]
            sigs.append([offset,size])
#            print offset,
            print size

        line = f.readline()

    return sigs


sigs1 = get_sigs(filename1)

'''
sigs2 = get_sigs(filename2)

match_len = len(sigs1)
if len(sigs2) < match_len:
    match_len = len(sigs2)

for sig in sigs1:
'''    



