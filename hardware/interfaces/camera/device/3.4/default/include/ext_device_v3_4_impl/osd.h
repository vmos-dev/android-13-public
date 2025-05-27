namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_4 {
namespace implementation {

#define PNG_LOGO "/vendor/etc/camera/logo.png"
#define FONT_FILE "/vendor/etc/camera/simhei.ttf"
#define OSD_TEXT L"Rockchip %04d年%02d月%02d日 %02d:%02d:%02d"

extern void processOSD(int width,int height,unsigned long dst_fd);


}  // namespace implementation
}  // namespace V3_4
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android
