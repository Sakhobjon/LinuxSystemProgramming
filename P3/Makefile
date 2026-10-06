HELP = ssu_help
EXT2 = ssu_ext2


$(EXT2) : $(EXT2).o $(HELP).o 
	$(CC) -g -o $@ $^

$(EXT2).o : $(EXT2).c
	$(CC) -c -o $@ $^
	
$(HELP).o : $(HELP).c
	$(CC) -c -o $@ $^
	
clean :
	rm -rf $(EXT2)
	rm -rf *.o

