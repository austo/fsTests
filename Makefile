CC = gcc

OBJDIR := out

CFLAGS = -Wall -Wextra -Wno-unused-parameter \
	-I/usr/local/include -L/usr/local/lib -luv

.PHONY: clean

clean:
	rm -f core $(OBJDIR)/* *.log

%: %.c $(OBJDIR)
	$(CC) $(CFLAGS) $< -o $(OBJDIR)/$@

$(OBJDIR):
	mkdir -p $(OBJDIR)
	