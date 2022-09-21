# E-Hentai Downloader 
download images from e-hentai.org or similar

# INSTALLATION
e-hentai-dl is very small. It is written in C without any external library or frameworks. 
You just need to download the zip package, or clone from github:

    git clone https://github.com/xiyue077/e-hentai-dl.git

then build and install by type:

    make
    make install

Though **e-hentai-dl** doesn't require external library to build, it requires the **wget** installed as its backend, 
which has been widely distributed in almost any linux like systems. 
Installation for example in Debian system:

    sudo apt-get install wget

or in Fedora system:

    sudo yum install wget

# DESCRIPTION

**e-hentai-dl** is a command-line program to download images from e-hentai.org. 
It's lightweight, fast to run, and not platform specific. 
It works on any Unix like systems or Windows/Cygwin/MinGW combination, as long as **wget** is available. 
It is released to the public domain, which means you can modify it, redistribute it or use it however you like.
 
    e-hentai-dl [OPTIONS] URL [URL...]

# OPTIONS

    -s, --single                         Download only one image and stop
    -p, --proxy URL                      Specify a proxy server
    -h, --help                           Print this help text and exit
    --version                            Print program version and exit
    --help-                              Print more helps of each extractor module (debug)
    -k, --keep-webpage                   Save the webpage for further study (debug)
    -d[a|i]                              Dump URL of [all|image] in the page (debug)

# SUPPORTED WEBSITE

 - e-hentai.org
 - hentaiera.com
 - comicporn.xxx
 - motherless.com
 - heavy-r.com
 - xvideos.com

# EXAMPLES

### Download all images from archive 2327820
```
e-hentai-dl https://e-hentai.org/g/2327820/ac144bcd8c/
```
It will create a directory 'ac144bcd8c' and store all images in it from that archive.

### Download since the 4th images from archive 2327820
```
e-hentai-dl https://e-hentai.org/s/b9af28e243/2327820-4
```
The URL points to '2327820-4' so it will downloaded from the 4th image to the end. 
All images will be stored in the current directory.

### Just download the 4th images from archive 2327820
```
e-hentai-dl -s https://e-hentai.org/s/b9af28e243/2327820-4
```



