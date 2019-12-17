all:
	-cd shared/Debug && make all 
	-cd SAC-server/Debug && make all
	-cd SAC-cli/Debug && make all
	-cd suse/Debug && make all
	-cd muse/Debug && make all
	-cd hilolay && make all
	-cd libmuse/Debug && make all 
	-export LD_LIBRARY_PATH=/home/utnso/tp-2019-2c-Vendo-FIAT-600/shared/Debug


clean:
	-cd shared/Debug && make clean
	-cd SAC-server/Debug && $(MAKE) clean
	-cd SAC-cli/Debug && $(MAKE) clean
	-cd suse/Debug && make clean
	-cd muse/Debug && make clean
	-cd hilolay && make clean

