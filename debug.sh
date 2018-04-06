

function check_package()
{
    #"libcv2.4  libhighgui2.4  libhighgui-dev libcv-dev  libopencv-contrib-dev libopencv-core-dev libopencv-core2.4v5 libopencv-dev libopencv-gpu-dev libopencv-gpu2.4v5 libopencv-imgproc-dev libopencv-video-dev  libopencv-video2.4v5 libopencv2.4-java libopencv2.4-jni"
    check_list="nmap libboost-all-dev curl-devel"
    for s in $check_list; do
        apt list --installed $s 2>&1 |grep $s > /dev/null
        if [ "$?" != "0" ]; then
            echo "--> Package $s not installed. Try installing..."
            sudo apt-get install -y $s
        fi
    done
}

check_package

export LOCAL_HOME=/home/tony
export LD_LIBRARY_PATH=".:${LOCAL_HOME}/bin/build_cv249/install/lib:${LD_LIBRARY_PATH}:./:./lib:./lib/HCNetSDKCom"
#export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:./:./lib:./lib/HCNetSDKCom"
export PKG_CONFIG_PATH="${LOCAL_HOME}/bin/build_cv249/install/lib/pkgconfig:${PKG_CONFIG_PATH}:/usr/local/lib/pkgconfig"

#make clean
make debug
./jsonAPI
