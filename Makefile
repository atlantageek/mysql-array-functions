BINDIR=/usr/lib

all:
	gcc -shared -fPIC -I/usr/include/mysql -o udf_arrayfunc.so udf_arrayfunc.c

install-dependency:
	mkdir -p $(BINDIR)

install: install-dependency
	cp -f udf_arrayfunc.so $(BINDIR)/udf_arrayfunc.so
