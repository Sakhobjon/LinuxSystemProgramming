HEADER = ssu_header
HELP = ssu_help
ADD = ssu_add
CLEANUP = ssu_cleanupd
SHOW = ssu_show


$(CLEANUP) : $(CLEANUP).o $(HEADER).o $(HELP).o $(ADD).o $(SHOW).o
	$(CC) -g -o $@ $^ -lcrypto -fcommon

$(CLEANUP).o : $(CLEANUP).c
	$(CC) -c -o $@ $^ -lcrypto -fcommon
	
$(HEADER).o : $(HEADER).c
	$(CC) -c -o $@ $^ -lcrypto -fcommon
	
$(HELP).o : $(HELP).c
	$(CC) -c -o $@ $^ -lcrypto -fcommon
	
$(ADD).o : $(ADD).c
#	$(CC) -c -o $@ $^ -lcrypto -fcommon

$(SHOW).o : $(SHOW).c
	$(CC) -c -o $@ $^ -lcrypto -fcommon
	

clean :
	rm -rf $(CLEANUP)
	rm -rf *.o

