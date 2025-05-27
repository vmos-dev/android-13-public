#ifndef __SUBVIDEO_H__
#define __SUBVIDEO_H__

#include <inttypes.h>
#include <linux/videodev2.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_4 {
namespace implementation {

#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define FMT_NUM_PLANES 1

#define BUFFER_COUNT 6


struct buffer {
    void *start;
    size_t length;
    int export_fd;
    int sequence;
};

typedef struct _demo_context {
char out_file[255];
char dev_name[255];
char dev_name2[255];
char sns_name[32];
int dev_using;
int width;
int height;
int format;
int fd;
enum v4l2_buf_type buf_type;
struct buffer *buffers;
unsigned int n_buffers;
int frame_count;
FILE *fp;

int writeFile;
int writeFileSync;
int limit_range;
struct buffer *buffers_mp;
int outputCnt;
int skipCnt;
void *pixels;
} demo_context_t;


extern demo_context_t main_ctx;
extern  void start_capturing(demo_context_t *ctx);
extern  void errno_exit(demo_context_t *ctx, const char *s);
extern  int xioctl(int fh, int request, void *arg);
extern  void uninit_device(demo_context_t *ctx);
extern  void stop_capturing(demo_context_t *ctx);
extern  void open_device(demo_context_t *ctx);
extern  void init_device(demo_context_t *ctx);
extern  void close_device(demo_context_t *ctx);

extern void processHdmiWithCamera(void* inData, int width,int height,int format,void* inData2,int width2,int height2,int format2);

}  // namespace implementation
}  // namespace V3_4
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android
#endif //__SUBVIDEO_H__