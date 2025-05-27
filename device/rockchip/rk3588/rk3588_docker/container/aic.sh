#!/bin/bash

### 默认配置，可根据需求进行修改
# Android容器配置文件地址
CONTAINER_CONFIG_DIR=/userdata/container/android_config
COMMON_CONFIG=$CONTAINER_CONFIG_DIR/container_common.conf
# docker daemon.json文件地址
DOCKER_DAEMON_FILE=/userdata/container/daemon.json
# 默认容器数量
CONTAINER_NUM=1
# 默认网络类型: macvlan_dhcp/macvlan_static/docker0/host
# 如需修改请修改android_config/container_common.conf或container_x.conf对应的network.type
NETWORK_TYPE=docker0
# macvlan网络名称
MACVLAN_NAME=macvlan

# 容器data地址
ANDROID_DATA_DIR=/userdata/container/android_data
# 容器data是否使用project quota： 默认没有配置，需要配置设置为true即可
CONTAINER_DATA_PRJQUOTA=
# 启用CONTAINER_DATA_PRJQUOTA后配置
CONTAINER_DATA_PRJQUOTA_LIMIT=10G

# 全局变量
DOCKER_IMAGE="test"
ANDROID_IP="192.168.12.2"
MAC_ADDR=""
CONTAINER_DATA_DIR="/userdata/android_data/data_0"


run_cmd() {
	cmd=$1
	echo "$(date +"%Y-%m-%d %H:%M:%S") $cmd"
	eval "$cmd"
	if [ $? -ne 0 ]; then
		echo "$(date +"%Y-%m-%d %H:%M:%S") $cmd error."
		exit 1
	fi
}

install_package() {
	package_name=$1

	if [ "$(dpkg -l | grep $package_name | awk '{print $2}')"x != "$package_name"x ]; then
		run_cmd "apt-get install $package_name -y"
	else
		if [ "$(dpkg -l | grep $package_name | awk '{print $1}')"x != "ii"x ]; then
			run_cmd "apt-get install $package_name -y"
		fi
	fi
}

# 解析配置文件，并将其中的键值对export为环境变量
# 输入参数：配置文件路径
parse_config() {
	config_file=$1
	# 检查配置文件是否存在
	if [ ! -f "$config_file" ]; then
		echo "配置文件 $config_file 不存在."
		exit 1
	fi

	# 逐行读取配置文件并设置变量
	while IFS= read -r line; do
		# 忽略注释和空行
		if [[ $line =~ ^\s*# || -z $line ]]; then
			continue
		fi

		# 使用等号（=）分隔键值对
		key=$(echo "$line" | cut -d= -f1)
		value=$(echo "$line" | cut -d= -f2)

		# 移除键和值中的空白字符,并将key的转换为大写
		key=$(echo "$key" | tr -d '[:space:]' | tr '[:lower:]' '[:upper:]')
		value=$(echo "$value" | tr -d '[:space:]')
		# 因为shell脚本环境变量不能包含“.”, 将key中的“.”转换为“_”
		key=$(echo "$key" | sed 's/\./_/g')

		# 将键值对设置为环境变量
		export "$key=$value"
	done < "$config_file"
	# export
}

# 判断输入的IMAGE_TAG是否存在
is_docker_image_exist(){
	IMAGE_TAG=$1

	ANDROID_IMAGE_TAG_LIST=$(docker images | grep -v REPOSITORY | awk -F" " '{print $2}')
	for ANDROID_IMAGE in $ANDROID_IMAGE_TAG_LIST;do
		if [ "$ANDROID_IMAGE"x == "$IMAGE_TAG"x ];then
			echo "Android image is exist"
			return 1
		fi
	done
	return 0
}

# 创建DOCKER IMAGE
# 需要输入一个参数：容器固件地址
creat_docker_image() {
	if [ $# -eq 1 ]; then
		SUPER_IMG=$1
	else
		echo "error: you must specify the path to android firmware"
		exit 1
	fi

	# 检查输入的容器固件地址是否存在
	if test -e "$SUPER_IMG"; then
	  echo "Found container firmware in: $SUPER_IMG"
	else
	  echo "ERROR: Container firmware is not exist"
	  return
	fi

	# 删除所有没有使用的docke images / network / container
	run_cmd "docker system prune -af"

	# 解压容器固件：主要就是super.img
	run_cmd "rm -rf super_img"
	run_cmd "mkdir super_img"
	run_cmd "tar -xvf $SUPER_IMG"

	# 挂载分区
	# mount system as root
	run_cmd "mkdir super_img/root"
	run_cmd "sudo mount super_img/system.img super_img/root -o rw"

	MOUNT_LIST=$(ls super_img| grep img)

	for i in $MOUNT_LIST; do
		mount_point="${i%*${i:(-4)}}"
		if [ "$mount_point"x == "system"x ]; then
			echo "system has mounted as root already"
		else
			echo "mount_point: $mount_point"
			run_cmd "sudo mount super_img/$i super_img/root/$mount_point -o rw"
		fi
	done

	# Android 容器无需再挂载分区，所以删掉fstab.rk30board
	# if [ -e "super_img/root/vendor/etc/fstab.rk30board" ]; then
	# 	sudo rm super_img/root/vendor/etc/fstab.rk30board
	# fi

	# 准备版本信息
	SOC_NAME=$(tr -d '\0' </sys/firmware/devicetree/base/compatible | awk -F, '{print $3}')
	VERSION=$(cat super_img/root/vendor/build.prop | grep ro.rksdk.version |awk -F= '{print $2}')
	BUILD_TIME=$(cat super_img/root/vendor/build.prop | grep ro.vendor.build.fingerprint | awk -F= '{print $2}'|awk -F/ '{print $5}' | awk -F: '{print $1}')

	# 判断Android image是否已经存在
	IMAGE_TAG="$VERSION-$BUILD_TIME"
	is_docker_image_exist $IMAGE_TAG
	IS_IMAGE_EXIST=$?

	if [ "$IS_IMAGE_EXIST"x == "0"x ]; then
		sudo tar --xattrs -c -C super_img/root . | sudo docker import -c 'ENTRYPOINT ["/init", "androidboot.hardware=rk30board"]' - $SOC_NAME:$VERSION-$BUILD_TIME
		#sudo docker save -o $SOC_NAME-$VERSION.img $SOC_NAME:$VERSION
		#sudo docker rmi $TARGET_PRODUCT:$VERSION
	else
		echo "Android image exit, do not load again!"
	fi

	# umount分区
	DOCKER_IMAGE=$SOC_NAME:$VERSION-$BUILD_TIME
	for i in $MOUNT_LIST; do
		mount_point="${i%*${i:(-4)}}"
		if [ "$mount_point"x == "system"x ]; then
			echo "systen mount as root, umount late"
		else
			echo "unmount: $mount_point"
			sudo umount super_img/root/$mount_point
		fi
	done
	sudo umount super_img/root

	sudo rm -rf super_img
}

# 准备Android容器的data目录
# 根据容器编号区分
# 输入参数： 容器编号
prepare_android_data() {
	CONTAINER_NO=$1
	CONTAINER_DATA_DIR="$ANDROID_DATA_DIR/data_$CONTAINER_NO"

	run_cmd "rm -rf $CONTAINER_DATA_DIR"
	run_cmd "mkdir -p $CONTAINER_DATA_DIR"

	# project quota 配额
	if [ "$CONTAINER_DATA_PRJQUOTA"x == "true"x ]; then
		echo "container data use project quota"
		run_cmd "apt install quota"
		PROJECT_ID="110$CONTAINER_NO"
		run_cmd "chattr +P -p $PROJECT_ID $CONTAINER_DATA_DIR"
		run_cmd "setquota -P $PROJECT_ID $CONTAINER_DATA_PRJQUOTA_LIMIT $CONTAINER_DATA_PRJQUOTA_LIMIT 0 0 /userdata"
	fi
}

# 获取网段中一个空闲ip：
# 注意，本机虚拟机的ip需要过滤掉
# 需要安装fping（apt install fping）
get_ip(){
	install_package "fping"

	IP_NETWORK_SEGMENT=$(ifconfig -a|grep inet|grep -v 127.0.0.1 | grep -v 172.17.0|grep -v inet6|awk '{print $2}' | awk -F"." '{print $1"."$2"."$3".0/24"}')
	IDLE_IP_LIST=$(fping -ugq $IP_NETWORK_SEGMENT)
	CONTAINER_ID_LIST=$(docker ps -a |grep -v "CONTAINER ID" | awk '{print $1}')
	for IP in $IDLE_IP_LIST; do
		for CONTAINER_ID in $CONTAINER_ID_LIST; do
			CONTAIN_IP=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' $CONTAINER_ID)
			if [ "$CONTAIN_IP"x == "$IP"x ]; then
				IP=""
				break
			fi
		done
		if [ "$IP"x != ""x ]; then
			break
		fi
	done
	ANDROID_IP=$IP
	echo "get a valid IP: $IP"
}

# 创建macvlan网络
creat_macvlan_network(){
	echo " creat macvlan network ..."
	# 获取所有网络接口的名称,wifi不支持macvlan
	all_interfaces=$(ip link show | awk -F': ' '{print $2}' | grep -v "lo" |  grep -v veth | grep -v docker | grep -v wlan)
	# 遍历每个网络接口并尝试连接到互联网, 默认使用第一个能联网的网络接口
	for interface in $all_interfaces; do
		if ping -c 1 -W 1 www.baidu.com -I $interface &> /dev/null; then
			PARENT=$interface
			SUB_NET=$(ip addr show dev $interface | awk '/inet /{print $2}' | awk -F"." '{print $1"."$2"."$3".0/24"}')
			break
		fi
	done

	# 物理机网卡设置为混杂模式
	run_cmd "ip link set $PARENT promisc on"
	run_cmd "docker network create -d macvlan --subnet=$SUB_NET -o macvlan_mode=bridge -o parent=$PARENT $MACVLAN_NAME"
}

# 随机Mac地址
random_mac_addr() {
	# 生成一个随机的mac地址
	# od -An -N6 -tx1 /dev/urandom：使用 /dev/urandom 从操作系统中生成6个随机字节
	# sed -e 's/^ *//'：去掉输出中每行前面可能的空格
	# sed -e 's/ */:/g'：将每个字节之间的空格替换为冒号，以匹配标准MAC地址格式
	# sed -e 's/:$//'：删除MAC地址末尾的冒号（如果有的话）
	# sed -e 's/^\(.\)[13579bdf]/\10/'：确保生成的MAC地址的最低有效位（LSB）的第二位是0，
	# 以表示这是一个本地MAC地址，而不是全局唯一的MAC地址
	MAC_ADDR=$(od -An -N6 -tx1 /dev/urandom | sed -e 's/^  *//' -e 's/  */:/g' -e 's/:$//' -e 's/^\(.\)[13579bdf]/\10/')
}

# 准备网络：是否使用macvlan， 使用静态or动态
# 静态网络是否指定了ip，没有则先获取有效ip
prepare_network() {
	IS_MACVLAN=$(echo $NETWORK_TYPE | awk -F"_" '{print $1}')
	echo "NETWORK_TYPE=$NETWORK_TYPE, IS_MACVLAN=$IS_MACVLAN"
	if [ "$IS_MACVLAN"x == "macvlan"x ]; then
		MACVLAN=$(docker network ls |awk '{print $3}' | grep macvlan)
		if [ "$MACVLAN"x != "macvlan"x ]; then
			creat_macvlan_network
		fi
	fi
	# macvlan 使用静态ip
	if [ "$NETWORK_TYPE"x == "macvlan_static"x ]; then
		# 配置文件中没有指定ip，需要获取一个空闲ip
		get_ip
	fi
	random_mac_addr
}

# 判断是否输入容器名是否已存在
is_container_name_exist() {
	CONTAINER_NAME=$1
	CONTAINER_ID=$(docker ps -a|grep $CONTAINER_NAME | awk '{print $1}')
	if [ "$CONTAINER_ID"x != ""x ]; then
		echo "container name $CONTAINER_NAME is exist: $CONTAINER_ID!"
		return 1
	fi
	return 0
}

# 执行docker run 命令运行容器
run_docker_image() {
	NAME=$1
	DATA_DIR=$2
	ANDROID_IMAGE=$3
	CONTAINER_CONFIG=$4
	CONTAINER_NO=$5

	# 准备docker run 参数
	CONTAINER_NAME="--name=$NAME"
	HOST_NAME="--hostname=$NAME"
	DATA_BIND="-v $DATA_DIR:/data"
	CONFIG_COMMON="-v $CONTAINER_CONFIG_DIR/container_common.conf:/vendor/etc/container/container_common.conf"
	CONFIG_PRIVATE="-v $CONTAINER_CONFIG_DIR/$CONTAINER_CONFIG:/vendor/etc/container/container.conf"
	PORT_BIND="-p 110$CONTAINER_NO:5555"
	CONTAINER_MAC="--mac-address=$MAC_ADDR"

	if [ "$NETWORK_TYPE"x == "macvlan_static"x ]; then
		CONTAINER_NETWORK="--network=$MACVLAN_NAME"
		CONTAINER_IP="--ip=$ANDROID_IP"
	elif [ "$NETWORK_TYPE"x == "macvlan_dhcp"x ]; then
		CONTAINER_NETWORK="--network=$MACVLAN_NAME"
		CONTAINER_IP=""
		PORT_BIND=""
	elif [ "$NETWORK_TYPE"x == "docker0"x ]; then
		# docker0 网络
		CONTAINER_NETWORK=""
		CONTAINER_IP=""
	elif [ "$NETWORK_TYPE"x == "host"x ]; then
		CONTAINER_NETWORK="--network=host"
		CONTAINER_IP=""
		# 使用host网络，不支持指定mac地址
		CONTAINER_MAC=""
	fi

	run_cmd "docker run -itd --restart=always --privileged $CONTAINER_NAME $HOST_NAME $DATA_BIND \
		$CONFIG_COMMON $CONFIG_PRIVATE \
		$CONTAINER_NETWORK $CONTAINER_MAC $CONTAINER_IP $PORT_BIND \
		$ANDROID_IMAGE"
}

# 更新容器固件：仅更新固件，不改变网络、data数据等
# 不支持Android跨版本升级
# 输入参数：
# 1、容器固件地址，必选；
# 2、容器名称或者容器ID, 可选。未指定时，升级全部。
update_container() {
	if [ $# -eq 2 ]; then
		NEW_IMAGE_PATH=$1
		CONTAINER_NAME=$2
	elif [ $# -eq 1 ]; then
		NEW_IMAGE_PATH=$1
		CONTAINER_NAME=""
	fi

	# 安装json解析器
	install_package "jq"

	# 1. 准备新的容器固件
	if test -e "$NEW_IMAGE_PATH"; then
		creat_docker_image $NEW_IMAGE_PATH
	fi
	# 2. 确定要升级的容器, 并停止容器
	UPDATE_CONTAINER_LIST=()
	if [ -n "$CONTAINER_NAME" ]; then
		is_container_name_exist $CONTAINER_NAME
		IS_CONTAINER_EXIST=$?
		if [ "$IS_CONTAINER_EXIST" == "1" ]; then
			run_cmd "docker stop $CONTAINER_NAME"
			# 如果输入是container_id，需要进行转换成container_name,便于后面查找容器编号：android_x 中的x就是容器编号
			UPDATE_CONTAINER_NAME=$(docker ps -a | grep $CONTAINER_NAME | awk '{print $NF}')
			UPDATE_CONTAINER_LIST+=($UPDATE_CONTAINER_NAME)
		else
			echo "FATAL: Not found $CONTAINER_NAME"
			exit 1
		fi
	else
		CONTAINER_LIST=$(docker ps -a | awk '{print $NF}' | sed 1d)
		for CONTAINER in $CONTAINER_LIST; do
			run_cmd "docker stop $CONTAINER"
			UPDATE_CONTAINER_LIST+=($CONTAINER)
		done
	fi

	# 3. 删除原来的容器并升级容器固件
	CONTAINER_NUM=${#UPDATE_CONTAINER_LIST[@]}
	echo "CONTAINER_NUM=$CONTAINER_NUM"
	for UPDATE_CONTAINER_NAME in ${UPDATE_CONTAINER_LIST[@]}; do
		ANDROID_IP=$(docker inspect $UPDATE_CONTAINER_NAME | jq -r '.[0].NetworkSettings.IPAddress')
		MAC_ADDR=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.MacAddress}}{{end}}' $UPDATE_CONTAINER_NAME)
		CONTAINER_NO=$(echo "$UPDATE_CONTAINER_NAME" | awk -F'_' '{print $2}')
		CONTAINER_DATA_DIR=$(docker inspect $UPDATE_CONTAINER_NAME | jq -r '.[0].HostConfig.Binds' | grep "/data" |awk -F':' '{print $1}' | tr -d '"')
		COTAINER_CONFIG="container_$CONTAINER_NO.conf"
		run_cmd "docker rm -f $UPDATE_CONTAINER_NAME"
		run_docker_image $UPDATE_CONTAINER_NAME $CONTAINER_DATA_DIR $DOCKER_IMAGE $COTAINER_CONFIG $CONTAINER_NO
	done
}

# 运行容器
# 输入两个参数：
# 1、固件地址（必选：或者DOCKER IMAGE: REPOSITORY:TAG）
# 2、容器数量（可选，没有配置默认为1）
run_container() {
	if [ $# -eq 2 ]; then
		CONTAINER_NUM=$2
		IMAGE_OR_PATH=$1
	elif [ $# -eq 1 ]; then
		CONTAINER_NUM=1
		IMAGE_OR_PATH=$1
	fi

	# 判断IMAGE_OR_PATH是否为一个文件
	# IMAGE_OR_PATH 为文件时，creat DOCKER IMAGE
	# IMAGE_OR_PATH 为非文件时，判断IMAGE是否存在
	if test -e "$IMAGE_OR_PATH"; then
		creat_docker_image $IMAGE_OR_PATH
	else
		# 判断输入的 IMAGE 是否存在
		IMAGE_TAG="$(echo "$IMAGE_OR_PATH" | cut -d: -f2)"
		is_docker_image_exist $IMAGE_TAG
		IS_IMAGE_EXIST=$?
		if [ "$IS_IMAGE_EXIST"x == "1"x ]; then
			echo "$IMAGE_OR_PATH IMAGE is exist"
			DOCKER_IMAGE="$IMAGE_OR_PATH"
		else
			echo "FATAL: Not found $IMAGE_OR_PATH, please check it"
			exit 1
		fi
	fi

	if [ "$CONTAINER_NUM" -le 0 ]; then
		echo "CONTAINER_NUM=$CONTAINER_NUM <= 0, set CONTAINER_NUM=1"
		CONTAINER_NUM=1
	fi
	for ((i = 0; i < ${CONTAINER_NUM}; i++)); do
		NEW_CONTAINER_NO=$i
		# 获取已经存在的Android容器序号并按大小排列
		EXIST_CONTAINER_NO_LIST=$(docker ps | grep -v "CONTAINER ID" | awk -F" "  '{print $NF}'|awk -F"_"  '{print $2}'|sort -n)
		for EXIST_CONTAINER_NO in $EXIST_CONTAINER_NO_LIST; do
			echo "EXIST_CONTAINER_NO=$EXIST_CONTAINER_NO, NEW_CONTAINER_NO=$NEW_CONTAINER_NO"
			if [ "$EXIST_CONTAINER_NO" -lt "$NEW_CONTAINER_NO" ];then
				continue
			fi
			if [ "$EXIST_CONTAINER_NO" == "$NEW_CONTAINER_NO" ];then
				NEW_CONTAINER_NO=$(($NEW_CONTAINER_NO + 1))
				continue
			fi
			break
		done
		echo "NEW_CONTAINER_NO=$NEW_CONTAINER_NO"
		CONTAINER_NAME="android"_$NEW_CONTAINER_NO
		COTAINER_CONFIG="container_$NEW_CONTAINER_NO.conf"
		parse_config $COMMON_CONFIG
		parse_config $CONTAINER_CONFIG_DIR/$COTAINER_CONFIG
		prepare_android_data $NEW_CONTAINER_NO
		prepare_network
		run_docker_image $CONTAINER_NAME $CONTAINER_DATA_DIR $DOCKER_IMAGE $COTAINER_CONFIG $NEW_CONTAINER_NO
	done
}

# 初始化设备环境
# 安装docker及必要的应用，配置docker环境等
init_device() {
	run_cmd "update-alternatives --set iptables /usr/sbin/iptables-legacy"
	# 安装必要软件包
	run_cmd "sudo apt update"
	install_package "fping"
	# 安装docker包
	install_package "docker.io"

	# 修改docker的默认存储至/data目录
	if [ ! -e "/etc/docker/daemon.json" ]; then
		mkdir -p /data/docker
		cp $DOCKER_DAEMON_FILE /etc/docker/daemon.json
		systemctl daemon-reload
		systemctl restart docker
	fi

	# 如果未配置wifibt， 需关掉rkwifibt服务，避免因为ko加载问题导致卡住
	# SDK 版本不同，wifibt服务有变化
	run_cmd "systemctl disable rkwifibt"
	run_cmd "systemctl disable wifibt-init.service"
}

# 打印帮助函数
print_help() {
    echo "用法: $0 [-h] [-i][-c firmware_path] [-r image_or_path [container_num] ] [-u update_image_or_path [container_name] ]"
    echo "      该脚本主要是用来在 Debian 系统上通过 docker 部署和运行Android容器。"
    echo "      首次部署环境，请先运行 ./aic.sh -i 命令进行设备的初始化。"
    echo "选项:"
    echo "  -h            显示帮助信息"
    echo "  -i            初始化设备环境：./aic.sh -i"
    echo "                安装docker及必须的应用包，初始化docker环境和容器运行环境等"
    echo "  -c            创建docker image: ./aic.sh -c firmware_path"
    echo "                    参数1：firmware_path: 必选参数，表示容器的固件地址"
    echo "                    例如： ./aic.sh -c /path/to/android_firmware.tgz"
    echo "  -r            运行容器: ./aic.sh -r image_or_path [container_num]"
    echo "                    参数1：image_or_path: 必选参数，表示容器的固件地址或者docker image（REPOSITORY:TAG）"
    echo "                    参数2：container_num: 可选参数，表示容器数量，未配置，默认为1个"
    echo "                    例如： ./aic.sh -r /path/to/android_firmware.tgz 3"
    echo "  -u            升级容器： ./aic.sh -u update_image_or_path [container_name/container_id]"
    echo "                    参数1：update_image_or_path: 必选参数，表示要升级的容器的固件地址或者image（REPOSITORY:TAG）"
    echo "                    参数2：container_name/container_id: 可选参数，要升级的容器名称或容器ID，未配置，默认全部升级"
    echo "                    例如： ./aic.sh -c /path/to/android_firmware.tgz android_0"
}

# 解析命令行选项和参数
parse_opts() {
	HAS_OPTION=false
	OPTION=()
	while getopts ":hvic:r:u:" opt; do
		case $opt in
			h)
				print_help
				HAS_OPTION=true
				exit 0
				;;
			i)
				echo "init device for run container"
				init_device
				HAS_OPTION=true
				;;
			v)
				echo "aic.sh version is v1.0.2"
				HAS_OPTION=true
				exit 0
				;;
			c)
				echo "creat docker images,  OPTARG is: $OPTARG"
				ANDROID_FIRMWARE_PATH=$OPTARG
				creat_docker_image $ANDROID_FIRMWARE_PATH
				HAS_OPTION=true
				;;
			r)
				OPTION+=("$OPTARG")
				shift $((OPTIND-1))
				OPTION+=("$@")
				echo "run docker images,  OPTARG is: ${OPTION[*]}"
				IMAGE_OR_PATH=${OPTION[0]}
				CONTAINER_NUM=${OPTION[1]}
				echo "IMAGE_OR_PATH=$IMAGE_OR_PATH, CONTAINER_NUM=$CONTAINER_NUM"
				run_container $IMAGE_OR_PATH $CONTAINER_NUM
				HAS_OPTION=true
				;;
			u)
				OPTION+=("$OPTARG")
				shift $((OPTIND-1))
				OPTION+=("$@")
				echo "update docker container images,  OPTARG is: ${OPTION[*]}"
				IMAGE_OR_PATH=${OPTION[0]}
				CONTAINER_NO=${OPTION[1]}
				echo "IMAGE_OR_PATH=$IMAGE_OR_PATH, CONTAINER_NO=$CONTAINER_NO"
				update_container $IMAGE_OR_PATH $CONTAINER_NO
				HAS_OPTION=true
				;;
			?)
				echo "无效选项: -$OPTARG" >&2
				print_help
				exit 1
				;;
			:)
				echo "选项 -$OPTARG 需要参数." >&2
				print_help
				exit 1
				;;
		esac
	done
	if [ "$HAS_OPTION" == false ]; then
		echo "无效选项，请检查输入参数:"
		echo "运行脚本，需要指明要进行的操作（-c/-r/-i...）"
		print_help
	fi
}

main() {
	# 如果没有提供任何参数，则打印帮助信息
	if [ $# -eq 0 ]; then
		print_help
		exit 0
	fi
	# parse_config $COMMON_CONFIG

	parse_opts "$@"
}

main "$@"
