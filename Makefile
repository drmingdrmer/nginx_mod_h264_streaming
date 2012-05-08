# vim:noexpandtab:sw=2 ts=2

.PHONY: all dist install clean
 
HOME=$(shell echo ~)
PWD=$(shell pwd)
NGINX=$(HOME)/nginx-0.8.33/
#NGINX=$(HOME)/nginx-0.7.33/
 
VERSION=`./version.sh`
DIST_NAME=nginx_mod_h264_streaming-$(VERSION)
 
all:
	cd $(NGINX) && ./configure --sbin-path=/usr/local/sbin --add-module=$(PWD) --with-debug --with-http_flv_module
	make --directory=$(NGINX)
 
install:
	make --directory=$(NGINX) install
 
clean:
	 -rm -rf $(DIST_NAME)
	 -rm *.slo *.la *.lo *.o *.tar *.tar.gz
 
dist:
	-rm -rf $(DIST_NAME)
	-mkdir $(DIST_NAME)
	-mkdir $(DIST_NAME)/src
	cp -t $(DIST_NAME) ../../LICENSE ../../README config Makefile version.sh
	cp src/* $(DIST_NAME)/src
	tar cvf $(DIST_NAME).tar $(DIST_NAME)/
	gzip -f --best $(DIST_NAME).tar
	@echo $(DIST_NAME).tar.gz generated.
	rm -f ../download/$(DIST_NAME).tar.gz
	mv -t ../../download $(DIST_NAME).tar.gz


