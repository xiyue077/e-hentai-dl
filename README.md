# E-Hentai Downloader 
download images from e-hentai.org.

# INSTALLATION
**e-hentai-dl** is very simple and small. It is written in C without any external library or frameworks. 
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
It's lightweight, easy to run, and not platform specific. 
It works on any Unix like systems or Windows/Cygwin/MinGW combination, as long as **wget** is available. 
It is released to the public domain, which means you can modify it, redistribute it or use it however you like.
To minimize the performance impact to the server, **e-hentai-dl** is limited in single thread with rest time.
When you modify/integrate/redistribute it, please keep this convention for other users sake.
 
    e-hentai-dl [OPTIONS] URL [URL...]

# OPTIONS

    -s, --single                         Download only one image and stop
    -m, --multi NUM                      Download specified number of images
    -u, --unsort                         Do not prefix the sorting number to images
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

### Download first 10 images from archive 2327820
```
e-hentai-dl -m 10 https://e-hentai.org/g/2327820/ac144bcd8c/
```
It will create a directory 'ac144bcd8c' and download first 10 images in it 
from that archive. 

### Download since the 4th images from archive 2327820
```
e-hentai-dl https://e-hentai.org/s/b9af28e243/2327820-4
```
The URL points to '2327820-4' so it would download from the 4th image to the end. 
All images will be stored in the current directory.

### Download from the 4th to 18th images from archive 2327820
```
e-hentai-dl -m 15 https://e-hentai.org/s/b9af28e243/2327820-4
```
The URL points to '2327820-4' so it would download 15 images from the 4th image,
which makes it 4th to 18th, included. All images will be stored in the current directory.

### Just download the 4th images from archive 2327820
```
e-hentai-dl -s https://e-hentai.org/s/b9af28e243/2327820-4
```
it would download the 4th image then quit.

### Download a batch of archives
You may group many archive URLs into a text file, one URL per line, then run **e-hentai-dl**.
Empty line, non-URL line or lines started with '#' would be ignored. For example
```
$ cat list.txt
https://e-hentai.org/g/2331553/e0a97343d3/
https://e-hentai.org/g/2331480/a0c370e168/
https://e-hentai.org/g/2331419/00ab7755a4/
https://e-hentai.org/g/2330996/e758fba3be/

$ e-hentai-dl list.txt
```
**e-hentai-dl** would download these URLs one by one.

### Find the missing images
Sometimes some images could be missing while downloading the archive. 
You will find information like this before **e-hentai-dl** exit:
```
Images Downloaded: 18
Images Missed:     2
```
Unfortunately there's no cheap way to fix that. The e-hentai image server simply didn't response 
at that time by that port. It would be auto-recovered by 10 minutes to 2 hours randomly. 
But **e-hentai-dl** could not wait. It skips that one and keep going. 

However the image can be found back manually. When **e-hentai-dl** runs, it generates a log file 'e-hentai.log'
in the current directory. Inspect it with a text editor, for example:
```
[20220920093130] SESSION START
[20220920124008] 9e9aed7fec : 40 downloaded; 0 missed
[20220920125227] 2178873-55 : failed to get 72997235_p0.jpg
[20220920132412] 565077e1ff : 278 downloaded; 1 missed
[20220920141158] 74660a12b5 : 11 downloaded; 0 missed
[20220920164506] 5969e70318 : 170 downloaded; 0 missed
[20220920173343] 85925fd958 : 313 downloaded; 0 missed
[20220920173635] 2062785-9 : failed to get 90866278_p0.jpg
[20220920175218] 4f4fac615c : 117 downloaded; 1 missed
[20220920190251] 3e697702f6 : 326 downloaded; 0 missed
[20220921092914] Images Downloaded: 941
[20220921092914] Images Missed:     2
```
which means in archive 565077e1ff it failed to downloaded 72997235_p0.jpg from page 2178873-55; 
in archive 4f4fac615c it failed to downloaded 90866278_p0.jpg from page 2062785-9. 
So you may run this command:
```
$ cd 565077e1ff
$ e-hentai-dl -s 2178873-55
```
and
```
$ cd 4f4fac615c
$ e-hentai-dl -s 2062785-9
```
to recollect them. It normally works well. If it still stuck, please try again in another day.

### Rename the archive and clean the mess
As mentioned above, sometimes some images may go missing. 
For manually recovering these images, **e-hentai-dl** has to keep the intermediate web pages like this:
```
$ e-hentai-dl https://e-hentai.org/g/2327820/ac144bcd8c/
$ ls ac144bcd8c
1_20220913_220147_img1_Merryweatherey_1569793488405032960.jpg
2_20220913_220147_img2_Merryweatherey_1569793488405032960.jpg
2327820-1
2327820-2
2327820-3
2327820-4
2327820-5
2327820-6
2327820-7
2327820-8
3_20220913_220147_img3_Merryweatherey_1569793488405032960.jpg
4_20220913_220147_img4_Merryweatherey_1569793488405032960.jpg
5_20220915_174625_img1_Merryweatherey_1570453999177834500.jpg
6_20220915_174625_img2_Merryweatherey_1570453999177834500.jpg
7_20220915_174625_img3_Merryweatherey_1570453999177834500.jpg
8_20220915_174625_img4_Merryweatherey_1570453999177834500.jpg
ac144bcd8c
ac144bcd8c.txt
```
These 2327820-1 to 2327820-8 were the intermediate files. 
If nothing missing, they ought to be deleted; alas, the archive name ac144bcd8c is not dude friendly.
I normally change it to a more meanfully name, like the title in web page. 
**e-hentai-dl** can do this in one line by the '-c' option:
```
$ e-hentai-dl -c ac144bcd8c
rename: ac144bcd8c  ->  [peach] [Merryweather (PeaCh88)] Fantasy comics
$ ls '[peach] [Merryweather (PeaCh88)] Fantasy comics'
1_20220913_220147_img1_Merryweatherey_1569793488405032960.jpg
2_20220913_220147_img2_Merryweatherey_1569793488405032960.jpg
3_20220913_220147_img3_Merryweatherey_1569793488405032960.jpg
4_20220913_220147_img4_Merryweatherey_1569793488405032960.jpg
5_20220915_174625_img1_Merryweatherey_1570453999177834500.jpg
6_20220915_174625_img2_Merryweatherey_1570453999177834500.jpg
7_20220915_174625_img3_Merryweatherey_1570453999177834500.jpg
8_20220915_174625_img4_Merryweatherey_1570453999177834500.jpg
ac144bcd8c
ac144bcd8c.txt
```

### Unsort the images
By default setting, **e-hentai-dl** will add a number prefix to every downloaded images,
so that would make the image order align with that in the web page. 
For example, in the above archive ac144bcd8c, the original names of these images
```
1_20220913_220147_img1_Merryweatherey_1569793488405032960.jpg
2_20220913_220147_img2_Merryweatherey_1569793488405032960.jpg
3_20220913_220147_img3_Merryweatherey_1569793488405032960.jpg
```
actually were
```
20220913_220147_img1_Merryweatherey_1569793488405032960.jpg
20220913_220147_img2_Merryweatherey_1569793488405032960.jpg
20220913_220147_img3_Merryweatherey_1569793488405032960.jpg
```
This setting can also avoid overwriting the images with the same name, which sometimes happens in e-hentai.org.
This setting can be disabled by '-u' option.
```
$ e-hentai-dl -u https://e-hentai.org/g/2327820/ac144bcd8c/
$ ls ac144bcd8c/
20220913_220147_img1_Merryweatherey_1569793488405032960.jpg  2327820-2
20220913_220147_img2_Merryweatherey_1569793488405032960.jpg  2327820-3
20220913_220147_img3_Merryweatherey_1569793488405032960.jpg  2327820-4
20220913_220147_img4_Merryweatherey_1569793488405032960.jpg  2327820-5
20220915_174625_img1_Merryweatherey_1570453999177834500.jpg  2327820-6
20220915_174625_img2_Merryweatherey_1570453999177834500.jpg  2327820-7
20220915_174625_img3_Merryweatherey_1570453999177834500.jpg  2327820-8
20220915_174625_img4_Merryweatherey_1570453999177834500.jpg  ac144bcd8c
2327820-1                                                    ac144bcd8c.txt
```
You will receive the images with the original names. If the images have the same name, only the last one remains.

