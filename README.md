## What is RRU?
RRU stands for RISC OS(Raspberry Pi Version) ROM file utility.
You can see resource file list and read/write resource files with RRU.
**Please note that this utility has been only tested with RISC OS 5.23 and 5.24 ROM image for Raspberry Pi.**
## What it can, and can't do?
### It can
- See list of resource files in ROM image.
- Read resource files from ROM image.
- Write new resource file data to ROM image.
### It can't
- RRU can't read/write anything other than resource files.
- RRU does not work with some resource files, unfortunatly :(
- RRU can't fully write a file (to ROM image) that is larger than original(See below).
### Note about writing files to ROM image
When RRU writes a local file to ROM image, RRU does not update file size. So, logical file size is always same.
When RRU meets a file with different size:
- If file is smaller than original, RRU writes the file and adds padding byte(zero) to fill the original file area.
- If file is bigger than original, RRU will still write to ROM image, but it will be truncated.
## Note about file listing
RRU looks for files with these root directories:
- Apps
- Discs
- Fonts
- Resources
- ThirdParty
## Build
RRU is written purely in C, and all you need to do is just compile rru.c.
If you are using GCC on Linux, command is:

```
gcc -o rru rru.c
```
## Using RRU
You can type ```rru``` to see help. And here are few examples(RISCOS.IMG is ROM image file):
- ```rru ls RISCOS.IMG```: Shows list of resource files in ROM image.
- ```rru read RISCOS.IMG intmetric0 Fonts.Corpus.Bold.IntMetric0```: Writes contents of Fonts.Corpus.Bold.IntMetric0(ROM resource) into local file intmetric0.
- ```rru write RISCOS.IMG Resources.BootFX.1920x1080 1920x1080.jpg```: Writes contents of 1920x1080.jpg(Local file) into ROM resource Resources.BootFX.1920x1080. This command can be used to replace RPi RISC OS's boot screen background. In fact, this utility was initially made for that purpose.
## Don't forget to backup!
Please backup your original RISC OS ROM image before using this utility. This utility can potentially make your ROM image unbootable.
## I know that this utility is kinda dirty :( ##
The way this app finds a file entry is kinda dirty, but it works for now.
