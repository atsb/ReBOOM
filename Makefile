SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
DEP = $(OBJ:.o=.d)
CFLAGS = -Wall -DUNIX -g -DINSTRUMENTED -DRANGECHECK
LDFLAGS = -lSDL2 -lSDL2_mixer -lSDL2_net -lm

reboom: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

-include $(DEP)

%.d: %.c
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	rm -f $(OBJ) $(DEP) reboom