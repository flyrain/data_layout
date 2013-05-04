import sys
filename1 = sys.argv[1]
filename2 = sys.argv[2]

f1 = open(filename1,'r')
f2 = open(filename2, 'r')


lines1 = f1.readlines()
lines2 = f2.readlines()


begin1 =0
begin1 =0

for i in  range(len(lines1)):
    content1 = lines1[i].split()[3]
    content2 = lines2[i].split()[3]
    if content1 != content2:
        print lines1[i], 
        print lines2[i]



               
