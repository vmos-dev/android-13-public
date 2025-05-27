#!/system/bin/sh

valid_interface=()
NAT_INTERFACE=""
NAT_IP=""
NAT_FREQ=""
IS_NETWORK_CHANGE=0

# 输出日志到Android的logcat
# 需要两个参数，第一个固定为$LINENO
# 第二个为要输出的日志
# 如：print_log $LINENO "print_log test"
print_log(){
	line_no=$1
	log=$2
	logcat="/system/bin/log -t host_network.sh\(line:$line_no\) $log"

	eval "$logcat"
}

# 使用host网络，需要重新配置tcp port，重启adbd，避免网络adb异常
# 这边tcp port选择与aic.sh中定义的一致
restart_adbd(){
	print_log $LINENO "restart adbd"
	container_no=$(getprop ro.container.container_id)
	tcp_port_pre="110"
	tcp_port=$tcp_port_pre$container_no
	# 默认server端口号是5037,使用host网络，会和宿主机adbd的server端口冲突
	server_port=$((5038+$container_no))
	print_log $LINENO "restart adbd tcp_port: $tcp_port"
	setprop service.adb.tcp.port $tcp_port
	setprop ro.container.adbd.server.port $server_port
	kill -9 `pgrep adb`
	start adbd
}

get_interface(){
	valid_interface=()
	all_interfaces=$(ip link show | awk -F': ' '{print $2}' | grep -v "lo" |  grep -v veth | grep -v docker | grep -v dummy0)
	for interface in $all_interfaces; do
		if ping -I $interface -c 1 -W 1 www.baidu.com &> /dev/null; then
			print_log $LINENO "foud valid interface: $interface"
			valid_interface+=($interface)
		fi
	done
}

set_nat_for_docker0(){
	interface=$1
	# get default gateway
	gateway=$(ip -4 route list table 0 | grep default | grep $interface | awk  '{print $3}')
	# get ip_addr: 172.16.21.199/24
	ip_addr=$(ip -4 addr show dev $interface | awk '/inet / {print $2}')
	NAT_INTERFACE=$interface
	NAT_IP=$ip_addr
	NAT_FREQ=$(iw dev $interface link | grep freq |awk '{print $2}')
	print_log $LINENO "interface=$interface, gateway=$gateway, ip_addr=$ip_addr, freq=$NAT_FREQ"

	ip rule add from all lookup main pref 9999
	ip r a default via $gateway dev $interface proto static t main
	ndc nat enable docker0 $interface 2 $ip_addr
	echo 1 > /proc/sys/net/ipv4/ip_forward

	restart_adbd
}

is_network_change(){
	interface=$1
	ip_addr=$(ip -4 addr show dev $interface | awk '/inet / {print $2}')

	if [ "$ip_addr" == "$NAT_IP" ]; then
		ping -I $interface -c 1 -W 1 www.baidu.com &> /dev/null
		if [ $? -ne 0 ] ; then
			IS_NETWORK_CHANGE=1
		fi
		# wifi切换频率（2.4G->5G）也有可能导致使用bridge网络的容器网络异常
		# 所以网络频率切换也视为网络变化
		FREQ=$(iw dev $interface link | grep freq |awk '{print $2}')
		if [ "$FREQ" == "$NAT_FREQ" ]; then
			IS_NETWORK_CHANGE=0
		else
			IS_NETWORK_CHANGE=1
		fi
	else
		IS_NETWORK_CHANGE=1
	fi
}

main(){
	while true; do
		get_interface
		if [ ${#valid_interface[@]} -eq 0 ]; then
			print_log $LINENO "Not found valid interface to connect network"
			sleep 5
		else
			first_interface=${valid_interface[0]}
			set_nat_for_docker0 $first_interface
			break
		fi
	done
	while true; do
		is_network_change $NAT_INTERFACE
		if [ $IS_NETWORK_CHANGE == "1" ] ; then
			print_log $LINENO "May network status change, will reset nat for docker0"
			get_interface
			if [ ${#valid_interface[@]} -eq 0 ]; then
				print_log $LINENO "Not found valid interface to connect network"
				NAT_IP=""
				NAT_FREQ=""
				sleep 3
				continue
			fi
			first_interface=${valid_interface[0]}
			set_nat_for_docker0 $first_interface
		fi
		# 执行adb root等命令可能导致adbd异常退出，原因待查，此为workround
		adbd_pid=$(pgrep adbd)
		if [ -z $adbd_pid ]; then
			restart_adbd
		fi
		sleep 3
	done
}

main "$@"
