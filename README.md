### 编译
在X86编译服务器上运行，系统版本不低于Ubuntu 22.04.4（推荐），内存不小于64GB。
$ ./build_docker_android.sh -A
$ sudo ./build_docker_tar.sh

### 运行
编译好后IMAGE目录下会生成对应的目录, 下面有打包好的tar, 例如
IMAGE/VCLOUD_ANDROID14_USER/DOCKERTARS/vcloud_android14_user_20250529.tar

将上面的文件adb push到宿主机板卡/data/下面。

adb shell进入宿主机，解压并创建docker镜像
# cd /data/
# docker load -i vcloud_android14_user_20250529.tar

创建macvlan网卡
# docker network create -d macvlan --subnet=192.168.50.0/24 --gateway=192.168.50.1 -o parent=eth0 my-macvlan-net

运行实例
# docker run -itd --name=android-01 --hostname=android-01 --privileged --memory=4G -v /data/android_data/android-01/data:/data --mac-address=02:42:c0:de:52:ef --ip=192.168.50.189 --network=my-macvlan-net vcloud_android14_user_tmp

### adb连接
$ adb connect 192.168.50.189:5555
$ adb -s 192.168.50.189:5555 shell
