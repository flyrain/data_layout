LIBS=-L./xed2/xed2-ia32/lib -lxed  
CFLAGS=-I./xed2/xed2-ia32/include
Glib=-I/usr/include/glib-2.0 -I/usr/lib/i386-linux-gnu/glib-2.0/include  -lglib-2.0
all:
	gcc  -o signa  searchGlobal.c globalStruct.c code_init.c getPgd.c md5c.c vtop.c pageMd5Comparison.c main.c mddriver.c memload.c memory.c determineOsVersion.c $(LIBS) $(CFLAGS) $(Glib)  -g 
clean:
	rm -f modhash
