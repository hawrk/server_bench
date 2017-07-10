
$(TARGET):$(OBJ)
	@echo -e  Linking $@
	@echo $(CXX) -o $@ $^ $(CFLAGS) $(LIB)
	@$(CXX) -o $@ $^ $(CFLAGS) $(LIB)
	#install $(TARGET) ../../bin/
%.o: %.cpp
	@echo -e Compiling $<
	@$(CXX) $(CFLAGS) -c -o $@ $< $(INC)  
%.o: %.c
	@echo -e Compiling $< ...
	@$(CC) $(CFLAGS) -c -o $@ $< $(INC)  
%.pb.o:%.pb.cc
	@echo -e Compiling $< ...
	@$(CXX) $(CFLAGS) -c -o $@ $< $(INC)  	 
clean:
	@rm -f $(OBJ) $(TARGET)
