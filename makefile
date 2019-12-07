all:
	-cd shared/Debug && make all 
	-cd SAC-server/Debug && make all
	-cd SAC-cli/Debug && make all
	-cd suse/Debug && make all
	-cd muse/Debug && make all
	-cd hilolay/Debug && make all
	-export LD_LIBRARY_PATH=/home/utnso/tp-2019-2c-Vendo-FIAT-600/shared/Debug


clean:
	-cd SAC-server/Debug && $(MAKE) clean
	-cd SAC-cli/Debug && $(MAKE) clean
