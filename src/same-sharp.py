#! /usr/bin/env python
import sys
#sys.setrecursionlimit(10000)
filename = sys.argv[1]
f = open(filename,'r')
lines = f.readlines()
max_index = int(sys.argv[2])
class_count =0
def classify(lines,i):
    if(i > max_index): return
    groups = {}
    for line in lines:
#        if line.__contains__(' 4 4 4 '): continue
        if line.startswith('0 4 '): continue
        array = line.split()
        first = int(array[i])
        if( groups.has_key(first)):
            groups.get(first).append(line)
        else:
            groups[first] = [line]

    for k  in sorted(groups.iterkeys()):
        v = groups.get(k)
        if i== max_index and len(v) == 6:
            print  "%d: %s" % (len(v), v[0])
            global class_count
            class_count = class_count + 1

        if len(v) > 1:
            classify(v,i+1)

'''
main entry
'''
classify(lines,0)
print class_count, "classes"


def find_class(lines):
    line_index =-1
    classes =[]
    for line in lines[line_index+1:50]:
        line_index = line_index +1
        array =line.split()
        pattern = ''
        for i in range(10):
            pattern = pattern + array[i] + " "
    
            #print pattern
            same_pattern =[]
            addrs =[array[10]]
            for subline in lines[line_index+1:]:
                if subline.__contains__(pattern):
                    same_pattern.append(subline.strip())

            if len(same_pattern) > 0:
                same_pattern.append(line.strip())
                classes.append(same_pattern)

    print 'classes',
    print len(classes)            
    for item in classes:
        print len(item),
        print item

