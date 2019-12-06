all: 
	-cd SAC-server/Debug && make all
	-cd SAC-cli/Debug && make all


clean:
	-cd SAC-server/Debug && $(MAKE) clean
	-cd SAC-cli/Debug && $(MAKE) clean
