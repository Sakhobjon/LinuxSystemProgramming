HEADER = ssu_header
HELP = ssu_help
TREE = ssu_tree
CLEANUP = ssu_cleanup
ARRANGE = ssu_arrange


$(CLEANUP) : $(CLEANUP).o $(HEADER).o $(HELP).o $(TREE).o $(ARRANGE).o
	$(CC) -g -o $@ $^ -lcrypto -fcommon

$(CLEANUP).o : $(CLEANUP).c
	$(CC) -c -o $@ $^ -lcrypto -fcommon
	
$(HEADER).o : $(HEADER).c
	$(CC) -c -o $@ $^ -lcrypto -fcommon
	
$(HELP).o : $(HELP).c
	$(CC) -c -o $@ $^ -lcrypto -fcommon
	
$(TREE).o : $(TREE).c
	$(CC) -c -o $@ $^ -lcrypto -fcommon

$(ARRANGE).o : $(ARRANGE).c
	$(CC) -c -o $@ $^ -lcrypto -fcommon
	

clean :
	rm -rf $(CLEANUP)
	rm -rf *.o

