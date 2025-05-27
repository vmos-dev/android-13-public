touch system.img
touch system_ext.img
touch vendor.img
touch product.img
touch odm.img
chmod +x init_wrapper
docker build -t $1 .
