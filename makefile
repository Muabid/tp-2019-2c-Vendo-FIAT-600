all: 
	-cd SAC-server/Debug && $(MAKE) all
	-cd SAC-cli/Debug && $(MAKE) all

clean:
	-cd SAC-server/Debug && $(MAKE) clean
	-cd SAC-cli/Debug && $(MAKE) clean
