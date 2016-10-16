# Parameters
DEVICE=$1
FILE=$2
FILESIZE=$(stat -c%s "$FLE")
BPP=3
PIX_FMT=bgr24
WIDTH=720
HEIGHT_PP=1280

# If temp/ is missing
if [ ! -d "temp/" ]; then
  mkdir temp/
else
  rm -rf temp/*
fi

# If file doesn't exist
if [ -e "$FILE" ]; then
    echo "$FILE doesn't exist"
    exit
fi

# Remove old image
if [ -e "splash_$DEVICE.img" ]; then
    rm splash_$DEVICE.img
fi

echo "Selected device is $DEVICE"
echo "File is $FILE - $FILESIZE"

ffmpeg -hide_banner -loglevel quiet -i $FILE -f rawvideo -vcodec rawvideo -pix_fmt $PIX_FMT -y "temp/pic1.raw" >> /dev/null
echo "Debug: Create FASTBOOT Image"
ffmpeg -hide_banner -loglevel quiet -i stock/$DEVICE/fastboot.png -f rawvideo -vcodec rawvideo -pix_fmt $PIX_FMT -y "temp/pic2.raw" >> /dev/null
echo "Debug: Create BATTERY Image"
ffmpeg -hide_banner -loglevel quiet -i stock/$DEVICE/battery.png -f rawvideo -vcodec rawvideo -pix_fmt $PIX_FMT -y "temp/pic3.raw" >> /dev/null
if [DEVICE = "jalebi"]; then
echo "Debug: Create DOWNLOAD Image"
ffmpeg -hide_banner -loglevel quiet -i stock/$DEVICE/download.png -f rawvideo -vcodec rawvideo -pix_fmt $PIX_FMT -y "temp/pic4.raw" >> /dev/null
fi

cat temp/pic*.raw > temp/pic.raw
rgb24_converter.exe -p 4 -w $WIDTH -h $HEIGHT_PP -o 512 -e 1 temp/pic.raw splash_$DEVICE.img
echo "Splash image created as splash_$DEVICE.img"
