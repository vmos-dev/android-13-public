该目录主要是配置一些容器参数，主要分为两部分，一个是公共部分container_common.conf， 一个容器私有部分container_X.conf。
希望在所有容器都生效的部分，可以添加在container_common.conf里面，只希望在X容器生效的，添加在container_X.conf。

该配置文件需要在docker run启动容器时将其映射到容器中/vendor/etc/container目录下：
-v $CONTAINER_CONFIG_DIR/container_common.conf:/vendor/etc/container/container_common.conf \
-v $CONTAINER_CONFIG_DIR/$CONTAINER_CONFIG:/vendor/etc/container/container.conf

在容器启动时，init进程会去读取这两个配置文件，将其配置成以"ro.container."开头的属性，然后各应用再根据自己的需求读取相应属性。

目前仅支持 键值对 的形式进行配置， 以“#”号开头的行表示注释行。


