// CM-SPLASH v1 Tool
// By GokulNC
// Inspired from RLimager1.2.c from here: http://forum.xda-developers.com/showthread.php?t=2764354
// Tested to be working on Yu devices: Yuphoria and Yunique..
// May work on other devices too that use CM-SPLASH v1
// (Just open splash.img using a Hex Editor in PC & look at first 2 lines to check)

/*
	splash.img Format:
	-------------------
	1. Splash Header (First 512 Bytes by default)
	2. Payload Data
	
	1. Splash Header Format:
		-------------------------------------------------------------
		Byte Range (Hex)	Description
		-------------------------------------------------------------
		00 - 0F				"CM-SPLASH!!"
		10 - 1F				"v1"
		20 - 23				Width of picture1 (Little Endian)
		24 - 27				Height of picture1 (Little Endian)
		28 - 2F				Zeros
		30 - 33				Length of Payload Data (Byte Count of picture1) (Little Endian)
		34 - 3B				Zeros
		3C - 3F				Offset of picture1 (Little Endian)
	
		Similarly repeat last 32 bytes for picture2 in 40 to 5F and so on....
	(Pointed out by @makers_mark here: http://forum.xda-developers.com/nexus-9/themes-apps/tool-splash-screen-flasher-v1-0-t2931575/post67365057#post67365057 )
	
	2. Payload Data:
		Each Consecuive 4 Bytes of Data denotes a contiguous color range in R,G,B,count (max value of count = 255 (0xFF)).
		See comments in decode() & encode() functions for more info.
		
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

unsigned int width = 720, height_per_pic = 1280;
unsigned long long int img_offset = 512;
enum devices_patch_id {
	general = 0,
	yuphoria = 1,
	yunique = 2,
};
enum devices_patch_id device = general;

unsigned long int convertToLittleEndian(unsigned long int n) {
        return ((n << 24) | ((n << 8) & 0x00ff0000) | ((n >> 8) & 0x0000ff00) | (n >> 24));
}

int SystemIsBigEndian() { //http://www.geeksforgeeks.org/little-and-big-endian-mystery/
        unsigned int i = 1;
        char * c = (char * ) & i;
        if ( * c) return 0; //Little Endian
        else return 1; //Big Endian
}

void decode_rgb24_rle() {

        unsigned char repeats, data[4];
        unsigned long color;

        /* The offset might denote the position till which the splash header occupies (which contains no pics)..
        Reading values from the header to find the resolution is not a good idea (header structure may vary for devices) & of no use in this context,
        because as of now just a long raw file containing all pics is all I want.. 
        If you really intend to read data values from header, feel free to write it ;) */

        if (img_offset != 0) {
                fseek(stdin, img_offset, SEEK_SET);
        }

        /*The first 3 bytes are used to store 8-bit R,G,B values.
        The fourth byte stores the count value indicating no. of times the RGB pixel should be repeated */

        while (read(0, data, 4) == 4) {

                if (data[3] > 255) {
                        continue;
                }
                color = ((data[0]) | (data[1] << 8) | (data[2]) << 16);
                for (repeats = 0; repeats < data[3]; repeats++) {
                        write(1, & color, 3);
                }
        }
}

void encode_rgb24_rle() {
        FILE * output = fopen("./output/splash_logs.txt", "w");
        unsigned char count, zero = 0;
        unsigned long int last, color, pixel_count, position, prev_pic, ppp, lil_end, long_zero = 0;
        unsigned long int byteCount[16], offset[16];
        char header[32] = "CM-SPLASH!!\0\0\0\0\0v1\0\0By GokulNC\0\0";
        int i, j, pic_no = 1, column = 0;
        ppp = width * height_per_pic; //no. of pixels per picture
        count = pixel_count = 0;

        //Writing 512 zero's to manually fill the header later.....
        for (position = 0; position < img_offset; position++) write(1, & zero, 1);
        prev_pic = position;

        //The following lines are just the reverse of decode(), explore yourself!

        while (!feof(stdin) && read(0, & color, 3) == 3) {

                ++column;
                if (column > width) column -= width;
                if (pixel_count == 0) {
                        fprintf(output, "Offset of Picture-%d: %ld (Hex: %x )\r\n", pic_no, prev_pic, prev_pic);
                        offset[pic_no] = prev_pic;
                }

                if (!count) {
                        last = color;
                        count = 1;
                } else {
                        if ((color == last) && (count != 0xFF)) {
                                count++;
                        } else {
                                write(1, & last, 3);
                                write(1, & count, 1);
                                position += 4;
                                last = color;
                                count = 1;
                        }
                }

                ++pixel_count;
                if (pixel_count >= ppp) { //A picture should have been processed by now; record its payload length & offset
                        pixel_count = 0;
                        write(1, & last, 3);
                        write(1, & count, 1);
                        count = 0;
                        position += 4;
                        byteCount[pic_no] = position - prev_pic;
                        fprintf(output, "Byte Count of Picture-%d: %d (Hex: %x )\r\n\r\n", pic_no, byteCount[pic_no], byteCount[pic_no]);
                        if ((device == yuphoria || device == yunique) && byteCount[pic_no] > 330000) //Patch (Warning) for Yuphoria
                                fprintf(output, "WARNING: Byte Count of Picture%d is %ld, which exceeds 330000 \r\n\r\n", pic_no, byteCount[pic_no]);
                        ++pic_no;
                        while (position % 512) {
                                write(1, & zero, 1);
                                ++position;
                        }
                        prev_pic = position;
                }
        }

        if (column < width) { //irregular picture
                fprintf(output, "WARNING: Picture%d seems to be irregular (or there's misalignment somewhere).\r\n");
                fprintf(output, "It's recommended to recheck the picture resolutions..\r\n\r\n");
                while (column < width) { //just write the previous color
                        if (count != 0xFF) {
                                count++;
                        } else {
                                write(1, & last, 3);
                                write(1, & count, 1);
                                position += 4;
                                last = color;
                                count = 1;
                        }
                        ++column;
                }
        }

        if (count) { //orphaned range of picture
                write(1, & last, 3);
                write(1, & count, 1);
                position += 4;
                byteCount[pic_no] = position - prev_pic;
                fprintf(output, "Byte Count of misaligned Picture-%ld: %d (Hex: %x )\r\n\r\n", pic_no, byteCount[pic_no], byteCount[pic_no]);
                if ((device == yuphoria || device == yunique) && byteCount[pic_no] > 330000) //Patch (Warning) for Yuphoria & maybe Yunique 
                        fprintf(output, "WARNING: Byte Count of Picture%d is %d, which exceeds 330000 \r\n\r\n", pic_no, byteCount[pic_no]);
                ++pic_no;
                while (position % 512) {
                        write(1, & zero, 1);
                        ++position;
                }
        }

        //Now finally write the Header into first 512 bytes
        fseek(stdout, 0, SEEK_SET);
        write(1, header, 32);
        position = 32;
        if (SystemIsBigEndian()) {
                for (i = 1; i < pic_no; ++i) {
                        if (i == 4 && device == yunique) { //Patch for Yunique
                                for (j = 1; j <= 32; j++) write(1, & zero, 1); //32bytes of zeros inbetween before download mode for Yunique, no fucking idea why...
                                position += 32;
                        }
                        lil_end = convertToLittleEndian(width);
                        write(1, & lil_end, 4);
                        lil_end = convertToLittleEndian(height_per_pic);
                        write(1, & lil_end, 4);
                        write(1, & long_zero, 4);
                        write(1, & long_zero, 4);
                        lil_end = convertToLittleEndian(byteCount[i]);
                        write(1, & lil_end, 4);
                        write(1, & long_zero, 4);
                        write(1, & long_zero, 4);
                        lil_end = convertToLittleEndian(offset[i]);
                        write(1, & lil_end, 4);
                        position += 32;
                }
        } else {
                for (i = 1; i < pic_no; ++i) {
                        if (i == 4 && device == yunique) { //Patch for Yunique
                                for (j = 1; j <= 32; j++) write(1, & zero, 1); //32bytes of zeros inbetween before download mode for Yunique, no fucking idea why...
                                position += 32;
                        }
                        write(1, & width, 4);
                        write(1, & height_per_pic, 4);
                        write(1, & long_zero, 4);
                        write(1, & long_zero, 4);
                        write(1, & byteCount[i], 4);
                        write(1, & long_zero, 4);
                        write(1, & long_zero, 4);
                        write(1, & offset[i], 4);
                        position += 32;
                }
        }

        while (position++ < 512) write(1, & zero, 1); //fill zeros

        fclose(output);
}

int set_device_settings() {
        switch (device) {
        case yuphoria:
        case yunique:
                width = 720;
                height_per_pic = 1280;
                break;
        case general:
        default:
                break;
        }
        return 0;
}

int usage(void) {
        fprintf(stderr, "\n\n\nUsage:\n\nrgb24_converter ([-w] width [-d] height_pp [-o] offset [-p] device_id [-d]|[-e] 1) < input_file > output_file\n\n");
        fprintf(stderr, "[-e] or [-d] is mandatory, one or the other\n\n");
        fprintf(stderr, "-d 1			To decode the RGB24 RLE image to raw image\n");
        fprintf(stderr, "-e 1			To encode raw image to RGB24 RLE image\n");
        fprintf(stderr, "-h (height)	Height per picture in the RAW file. Default is 1280\n");
        fprintf(stderr, "-w (width)		Width of the image. Default is 720\n");
        fprintf(stderr, "-o (offset)	Offset (in bytes) to start decoding.  Default is 512\n");
        fprintf(stderr, "-p (device_id)	Patch to apply for specific devices. Default is 0.\n");
        fprintf(stderr, "\nExample Usage 1 -\nTo encode a picture.raw to splash.img (RGB24 RLE) & apply patch for device_id=2 (Yu Yunique):\n");
        fprintf(stderr, "rgb24_converter -p 2 -e 1 <picture.raw >splash.img\n");
        fprintf(stderr, "\nExample Usage 2 -\nTo decode a splash.img (RGB24 RLE) to picture.raw with offset 512:\n");
        fprintf(stderr, "rgb24_converter -o 512 -d 1 <picture.raw >splash.img");
        fprintf(stderr, "\nExample Usage 3 -\nTo encode a picture.raw to splash.img (RGB24 RLE) with width 1080 and height_pp 1920 :\n");
        fprintf(stderr, "rgb24_converter -w 1080 -h 1920 -e 1 <picture.raw >splash.img\n");
        return (1);
}

int main(int argc, char * * argv) {
        int decode_opt = 0, encode_opt = 0, c;
        char * param;

        while ((c = getopt(argc, argv, "w:h:o:e:d:p:")) != -1)
                switch (c) {

                case 'w':
                        param = optarg;
                        width = atoi(param);
                        break;

                case 'h':
                        param = optarg;
                        height_per_pic = atoi(param);
                        break;

                case 'o':
                        param = optarg;
                        img_offset = atoll(param);
                        break;

                case 'e':
                        param = optarg;
                        if (atoi(param)) encode_opt = 1;
                        break;

                case 'd':
                        param = optarg;
                        if (atoi(param)) decode_opt = 1;
                        break;

                case 'p':
                        param = optarg;
                        device = atoi(param);
                        set_device_settings();
                        break;
                }

        if (encode_opt ^ decode_opt) {
                fprintf(stderr, "\n\n\nWidth: %d\nHeight per picture: %d\nOffset: %d\n\n", width, height_per_pic, img_offset);
                if (encode_opt) {
                        fprintf(stderr, "Encoding raw image to RGB24 RLE image.......\n\n");
                        encode_rgb24_rle();
                        fprintf(stderr, "Successfully Encoded...\n\n");
                        return 0;
                } else {
                        fprintf(stderr, "Decoding RGB24 RLE image to raw image.......\n\n");
                        decode_rgb24_rle();
                        fprintf(stderr, "Successfully Decoded...\n\n");
                        return 0;
                }
        } else usage();

        return 1;
}
