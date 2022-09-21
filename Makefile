

#CFLAGS	= -Wall -g
CFLAGS	= -Wall -O3

TARGET	= e-hentai-dl

XTRACTR	= x_comicporn.c x_e_hentai.c  x_heavy_r.c  x_hentaiera.c  x_motherless.c  x_xvideos.c
COMMON	= urltool.c

all: $(TARGET)

e-hentai-dl: main.c $(XTRACTR) $(COMMON)
	gcc $(CFLAGS) -o $@ $^

motherless: x_motherless.c $(COMMON)
	gcc $(CFLAGS) -DMOTHERLESS_MAIN -o $@ $^

install:
	install -s $(TARGET) /usr/local/bin

clean:
	rm -f $(TARGET)
