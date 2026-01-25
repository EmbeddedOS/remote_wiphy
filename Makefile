all:
	make -C host
	make -C gadget

clean:
	make -C host clean
	make -C gadget clean