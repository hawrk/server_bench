all:
	tar -zxvf ./exlib/pb.tgz -C ./exlib
	make all -C ./comm/

clean:
	rm ./exlib/pb -rf
	make clean -C ./comm/

all32:
	make -e ARCH=32
