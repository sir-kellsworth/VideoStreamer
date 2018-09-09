so far, along with this project... this is the only thing you need to run
apt-get install libgstreamer1.0-0 gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-doc gstreamer1.0-tools gstreamer1.0-vaapi

for raspberry pi, you need a special library to make it work...
first run this command
sudo apt-get install autoconf automake libtool pkg-config libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libraspberrypi-dev

then download the git repository
git clone https://github.com/thaytan/gst-rpicamsrc.git

then config and make it
./autogen.sh --prefix=/usr --libdir=/usr/lib/arm-linux-gnueabihf/
make
sudo make install

now you can make project-remoteVideo
