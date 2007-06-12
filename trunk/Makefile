
#--------------------------------------------------------------------

CC = gcc
AR = ar cru
CFLAGS = -Wall -D_REENTRANT -D_GNU_SOURCE -g -fPIC
SOFLAGS = -shared
LDFLAGS = -lstdc++ -lpthread

LINKER = $(CC)
LINT = lint -c
RM = /bin/rm -f

ifeq ($(origin version), undefined)
version = 0.1
endif

#--------------------------------------------------------------------

LIBOBJS = spdictionary.o \
	spbtreeimpl.o spslistimpl.o \
	sparrayimpl.o spbstreeimpl.o sprbtreeimpl.o \
	spcache.o

TARGET =  libspdict.so testdict testcache

#--------------------------------------------------------------------

all: $(TARGET)

libspdict.so: $(LIBOBJS)
	$(LINKER) $(SOFLAGS) $^ -o $@

testdict: testdict.o
	$(LINKER) $(LDFLAGS) $^ -L. -lspdict -o $@

testcache: testcache.o
	$(LINKER) $(LDFLAGS) $^ -L. -lspdict -o $@

dist: clean spdict-$(version).src.tar.gz

spdict-$(version).src.tar.gz:
	@find . -type f | grep -v CVS | grep -v .svn | sed s:^./:spdict-$(version)/: > MANIFEST
	@(cd ..; ln -s spdict spdict-$(version))
	(cd ..; tar cvf - `cat spdict/MANIFEST` | gzip > spdict/spdict-$(version).src.tar.gz)
	@(cd ..; rm spdict-$(version))

clean:
	@( $(RM) *.o vgcore.* core core.* $(TARGET) )

#--------------------------------------------------------------------

# make rule
%.o : %.c
	$(CC) $(CFLAGS) -c $^ -o $@	

%.o : %.cpp
	$(CC) $(CFLAGS) -c $^ -o $@	

