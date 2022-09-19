

#CFLAGS	= -Wall -g
CFLAGS	= -Wall -O3

TARGET	= e-hentai-dl

XTRACTR	= x_comicporn.c x_e_hentai.c  x_heavy_r.c  x_hentaiera.c  x_motherless.c  x_xvideos.c

all: $(TARGET)

e-hentai-dl: main.c urltool.c $(XTRACTR)
	gcc $(CFLAGS) -o $@ $^

motherless: x_motherless.c urltool.c
	gcc $(CFLAGS) -DMOTHERLESS_MAIN -o $@ $^

install:
	cp $(TARGET) ~/bin

clean:
	rm -f $(TARGET)
