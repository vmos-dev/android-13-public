### 编译
`./build_docker_android.sh -A`

### 运行
编译好后IMAGE目录下会生成对应的目录, 找到对应时间戳的目录, 下面有打包好的tar, 例如

`IMAGE/VCLOUD_ANDROID13_USER/IMAGES/container/vcloud-android13-user.tgz`

将上面的文件`adb push`到`/data/local/tmp/`下面

解压并创建docker镜像
```
cd /data/local/tmp/
tar -xvf vcloud-android13-user.tgz
cd super_img
./build.sh android13_docker
```

创建macvlan网卡
```
docker network create -d macvlan \
  --subnet=192.168.10.0/24 \
  --gateway=192.168.10.1 \
  -o parent=eth0 \
  my-macvlan-net
```

运行实例
```
docker run -itd --name=android-01 \
--hostname=android-01 \
--privileged \
--memory=4G \
-v /data/android_data/android-01/data:/data \
--network=my-macvlan-net \
android13_docker
```

### adb连接
查找实例ip

如果只启动一个容器
```
cat /proc/`ps axj | grep init | grep second | awk '{ print $2 }'`/root/proc/1/net/fib_trie | awk '/32 host/ { print f } {f=$2}' 
```

如果有多个容器
```
cat /proc/[pid]/root/proc/1/net/fib_trie | awk '/32 host/ { print f } {f=$2}' 
[pid]为容器中任意进程的pid
```

例如
```
root@marsbox:/# cat /proc/`ps axj | grep init | grep second | awk '{ print $2 }'`/root/proc/1/net/fib_trie | awk '/32 host/ { print f } {f=$2}' 
127.0.0.1
192.168.10.2
```
查找到的ip是`192.168.10.2`, 就可以`adb connect 192.168.10.2`连接容器实例
