~~CUSTOM SPLASH SCREEN CREATOR for YU Yuphoria & Yunique~~

**How To Use:**
1. Make sure you have a picture for your custom splash with:
i. Resolution: 720x1280 for Yuphoria/Yunique
ii. PNG or JPG format
iii. Picture size less than 250kB
iv. less colorful

*(Tip: Try compressPNG.com to reduce no. of colors in PNG picture.
Upload your picture, click Settings, reduce number of colors to 16 or below & download it)*

2. Zip download this repo

3. Open a terminal and do whether
'''
./create_logo.sh DEVICE LOGO.png
'''

4. After the custom splash image (filename.img) is created, the output directory will be opened..
(Also open the file splash_logs.txt & ensure there are no warning/corruption messages.
If there's warning, reduce no. of colors in picture & try again from step-3)

5. Transfer the filename.img to the device's internal storage & flash it using Terminal Emulator like:
```
dd if=/sdcard/filename.img of=/dev/block/bootdevice/by-name/splash
```

**Credits:**
- Inspired from RLimager Created By makers_mark @ XDA (Link: http://forum.xda-developers.com/showthread.php?t=2764354# )
- Created for Yutopia, Yuphoria & Yunique By @GokulNC

**Advanced Information:**

- Pixel Format: bgr24 variant (with count byte after color bytes)
- Line Length (Width): 720
- RLE I/O Format: 3 bytes per pixel
- Max Run Length: 255 pixels
- Offset: 512 (First 512bytes for Header)
