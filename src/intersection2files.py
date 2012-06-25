import os
import sys
import myLogger

#get the logger
logger = myLogger.logger


def doIntersection(filename1,filename2):
    fixedfunctions=[]
    #open the min file, and add fixed function to array
    f = open(filename2, 'r')
    for line in f:
        fixedfunctions.append(line) #        print line
    f.close()
    
    #iterate all file
    intersection=[]
    
    #skip the empty file
    fileSize = os.path.getsize(filename1)
    if fileSize ==0:
        logger.info('%s is empty!',filename1)
        return []
    
    f = open(filename1, 'r')
    for line in f:
        if fixedfunctions.__contains__(line):
            intersection.append(line)
    f.close()
    
    fixedfunctions= intersection
    
    return fixedfunctions




def intersection():
    if len(sys.argv) < 4:
        logger.error("Please input original directory and kernel version")
        return
    
    filename1 = sys.argv[1]
    filename2 = sys.argv[2]    
    outputfile = sys.argv[3]

                
    fixedfunctions = doIntersection(filename1,filename2)
         
    f = open(outputfile, 'w')
    for fixedfunction in fixedfunctions:
        f.write(fixedfunction)
    
    f.close()
    
    logger.info('Intersection done. We have %d fixed function in intersection.',len(fixedfunctions))
    
if __name__ == "__main__":
    intersection();

