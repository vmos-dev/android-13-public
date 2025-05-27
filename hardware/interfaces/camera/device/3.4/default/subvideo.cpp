#include "subvideo.h"

#include <log/log.h>
#include <sys/mman.h>
#include <stdlib.h>

#include <RockchipRga.h>
#include <im2d_api/im2d.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferAllocator.h>
#include <ui/GraphicBufferMapper.h>



namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_4 {
namespace implementation {
 


 demo_context_t main_ctx = {
        .out_file = {'\0'},
        .dev_name = {'\0'},
        .dev_name2 = {'\0'},
        .sns_name = {'\0'},
        .dev_using = 1,
        .width = 640,
        .height = 480,
        .format = V4L2_PIX_FMT_MJPEG,
        .fd = -1,
        .buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .buffers = NULL,
        .n_buffers = 0,
        .frame_count = -1,
        .fp = NULL,
        .writeFile = 0,
        .writeFileSync = 0,
        .limit_range = 0,
        .buffers_mp = NULL,
        .outputCnt = 3,
        .skipCnt = 30,
        .pixels = nullptr,
    };
char* get_sensor_name(demo_context_t* ctx)
{
    return ctx->sns_name;
}



void errno_exit(demo_context_t *ctx, const char *s)
{
    ALOGE("%s: %s error %d, %s\n", get_sensor_name(ctx), s, errno, strerror(errno));
    //exit(EXIT_FAILURE);
}

int xioctl(int fh, int request, void *arg)
{
    int r;
    do {
        r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);
    return r;
}
void start_capturing(demo_context_t *ctx)
{
    ALOGE("@%s",__FUNCTION__);
    unsigned int i;
    enum v4l2_buf_type type;

    for (i = 0; i < ctx->n_buffers; ++i) {
        struct v4l2_buffer buf;

        CLEAR(buf);
        buf.type = ctx->buf_type;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == ctx->buf_type) {
            struct v4l2_plane planes[FMT_NUM_PLANES];

            buf.m.planes = planes;
            buf.length = FMT_NUM_PLANES;
        }
        if (-1 == ioctl(ctx->fd, VIDIOC_QBUF, &buf))
            errno_exit(ctx, "VIDIOC_QBUF");
    }
    type = ctx->buf_type;
    ALOGE("%s:-------- stream on output -------------\n", get_sensor_name(ctx));

    if (-1 == ioctl(ctx->fd, VIDIOC_STREAMON, &type))
        errno_exit(ctx, "VIDIOC_STREAMON");
}
char* get_dev_name(demo_context_t* ctx)
{
    if (ctx->dev_using == 1)
        return ctx->dev_name;
    else if(ctx->dev_using == 2)
        return ctx->dev_name2;
    else {
        ALOGE("!!!dev_using is not supported!!!");
        return NULL;
    }
}


void init_mmap(int pp_onframe, demo_context_t *ctx)
{
    struct v4l2_requestbuffers req;
    int fd_tmp = -1;

    CLEAR(req);

    fd_tmp = ctx->fd;

    req.count = BUFFER_COUNT;
    req.type = ctx->buf_type;
    req.memory = V4L2_MEMORY_MMAP;

    struct buffer *tmp_buffers = NULL;

    if (-1 == ioctl(fd_tmp, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            ALOGE("%s: %s does not support "
                "memory mapping\n", get_sensor_name(ctx), get_dev_name(ctx));
            //exit(EXIT_FAILURE);
        } else {
            errno_exit(ctx, "VIDIOC_REQBUFS");
        }
    }

    if (req.count < 2) {
        ALOGE("%s: Insufficient buffer memory on %s\n", get_sensor_name(ctx),
            get_dev_name(ctx));
        //exit(EXIT_FAILURE);
    }

    tmp_buffers = (struct buffer*)calloc(req.count, sizeof(struct buffer));

    if (!tmp_buffers) {
        ALOGE("%s: Out of memory\n", get_sensor_name(ctx));
        //exit(EXIT_FAILURE);
    }

    if (pp_onframe)
        ctx->buffers_mp = tmp_buffers;
    else
        ctx->buffers = tmp_buffers;

    for (ctx->n_buffers = 0; ctx->n_buffers < req.count; ++ctx->n_buffers) {
        struct v4l2_buffer buf;
        struct v4l2_plane planes[FMT_NUM_PLANES];
        CLEAR(buf);
        CLEAR(planes);

        buf.type = ctx->buf_type;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = ctx->n_buffers;

        if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == ctx->buf_type) {
            buf.m.planes = planes;
            buf.length = FMT_NUM_PLANES;
        }

        if (-1 == ioctl(fd_tmp, VIDIOC_QUERYBUF, &buf))
            errno_exit(ctx, "VIDIOC_QUERYBUF");

        if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == ctx->buf_type) {
            tmp_buffers[ctx->n_buffers].length = buf.m.planes[0].length;
            tmp_buffers[ctx->n_buffers].start =
                mmap(NULL /* start anywhere */,
                     buf.m.planes[0].length,
                     PROT_READ | PROT_WRITE /* required */,
                     MAP_SHARED /* recommended */,
                     fd_tmp, buf.m.planes[0].m.mem_offset);
        } else {
            tmp_buffers[ctx->n_buffers].length = buf.length;
            tmp_buffers[ctx->n_buffers].start =
                mmap(NULL /* start anywhere */,
                     buf.length,
                     PROT_READ | PROT_WRITE /* required */,
                     MAP_SHARED /* recommended */,
                     fd_tmp, buf.m.offset);
        }

        if (MAP_FAILED == tmp_buffers[ctx->n_buffers].start)
            errno_exit(ctx, "mmap");

        // export buf dma fd
        struct v4l2_exportbuffer expbuf;
        CLEAR (expbuf);
        expbuf.type = ctx->buf_type;
        expbuf.index = ctx->n_buffers;
        expbuf.flags = O_CLOEXEC;
        if (ioctl(fd_tmp, VIDIOC_EXPBUF, &expbuf) < 0) {
            errno_exit(ctx, "get dma buf failed\n");
        } else {
            ALOGE("%s: get dma buf(%d)-fd: %d\n", get_sensor_name(ctx), ctx->n_buffers, expbuf.fd);
        }
        tmp_buffers[ctx->n_buffers].export_fd = expbuf.fd;
    }
}

void init_device(demo_context_t *ctx)
{
    ALOGE("@%s",__FUNCTION__);
    struct v4l2_capability cap;
    struct v4l2_format fmt;

    if (-1 == ioctl(ctx->fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            ALOGE("%s: %s is no V4L2 device\n", get_sensor_name(ctx),
                get_dev_name(ctx));
            //exit(EXIT_FAILURE);
        } else {
            errno_exit(ctx, "VIDIOC_QUERYCAP");
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) &&
            !(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)) {
        ALOGE("%s: %s is not a video capture device, capabilities: %x\n",
            get_sensor_name(ctx), get_dev_name(ctx), cap.capabilities);
        //exit(EXIT_FAILURE);
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        ALOGE("%s: %s does not support streaming i/o\n", get_sensor_name(ctx),
            get_dev_name(ctx));
        //exit(EXIT_FAILURE);
    }

    if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) {
        ctx->buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        CLEAR(fmt);
        fmt.type = ctx->buf_type;
        fmt.fmt.pix.width = ctx->width;
        fmt.fmt.pix.height = ctx->height;
        fmt.fmt.pix.pixelformat = ctx->format;
        fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
        if (ctx->limit_range)
            fmt.fmt.pix.quantization = V4L2_QUANTIZATION_LIM_RANGE;
        else
            fmt.fmt.pix.quantization = V4L2_QUANTIZATION_FULL_RANGE;
    } else if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE) {
        ctx->buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        CLEAR(fmt);
        fmt.type = ctx->buf_type;
        fmt.fmt.pix_mp.width = ctx->width;
        fmt.fmt.pix_mp.height = ctx->height;
        fmt.fmt.pix_mp.pixelformat = ctx->format;
        fmt.fmt.pix_mp.field = V4L2_FIELD_INTERLACED;
        if (ctx->limit_range)
            fmt.fmt.pix_mp.quantization = V4L2_QUANTIZATION_LIM_RANGE;
        else
            fmt.fmt.pix_mp.quantization = V4L2_QUANTIZATION_FULL_RANGE;
    }

    if (-1 == ioctl(ctx->fd, VIDIOC_S_FMT, &fmt))
        errno_exit(ctx, "VIDIOC_S_FMT");

    init_mmap(false, ctx);
}

void stop_capturing(demo_context_t *ctx)
{
    ALOGE("@%s",__FUNCTION__);
    enum v4l2_buf_type type;

    type = ctx->buf_type;
    if (-1 == xioctl(ctx->fd, VIDIOC_STREAMOFF, &type))
        errno_exit(ctx, "VIDIOC_STREAMOFF");
}

void uninit_device(demo_context_t *ctx)
{
    ALOGE("@%s",__FUNCTION__);
    unsigned int i;
    if (ctx->n_buffers == 0)
        return;

    for (i = 0; i < ctx->n_buffers; ++i) {
        if (-1 == munmap(ctx->buffers[i].start, ctx->buffers[i].length))
            errno_exit(ctx, "munmap");

        close(ctx->buffers[i].export_fd);
    }

    free(ctx->buffers);
    ctx->n_buffers = 0;
}

void close_device(demo_context_t *ctx)
{
    ALOGE("@%s",__FUNCTION__);
    if (-1 == close(ctx->fd))
        errno_exit(ctx, "close");

    ctx->fd = -1;
}

void open_device(demo_context_t *ctx)
{
    ALOGE("-------- open output dev_name:%s -------------\n", get_dev_name(ctx));
    ctx->fd = open(get_dev_name(ctx), O_RDWR /* required */ /*| O_NONBLOCK*/, 0);

    if (-1 == ctx->fd) {
        ALOGE("Cannot open '%s': %d, %s\n",
            get_dev_name(ctx), errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
}



void processHdmiWithCamera(void* inData, int width,int height,int format,void* inData2,int width2,int height2,int format2){
    static void *pixels = nullptr;
    if(pixels == nullptr){
        static buffer_handle_t memHandle = nullptr;
        android::GraphicBufferAllocator& alloc(android::GraphicBufferAllocator::get());
        const auto usage =
            GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_SW_READ_RARELY | GRALLOC_USAGE_SW_WRITE_OFTEN;
        unsigned pixelsPerLine;
        android::status_t result =
                alloc.allocate(width, height, HAL_PIXEL_FORMAT_YCrCb_NV12, 1, usage, &memHandle, &pixelsPerLine, 0,
                                "ExternalCameraDevice");
        GraphicBufferMapper &mapper = GraphicBufferMapper::get();
        mapper.lock(memHandle,
                    GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_SW_READ_NEVER,
                    android::Rect(width, height),
                    (void **) &pixels);

        // If we failed to lock the pixel buffer, we're about to crash, but log it first
        if (!pixels) {
            ALOGE("Camera failed to gain access to image buffer for writing");
        }
    }

    rga_buffer_t srcA={};
    rga_buffer_t srcB={};
    rga_buffer_t dst={};

    im_rect srect={};
    im_rect drect={};
    im_rect prect={};
    int usage = 0;

    srcA.width = width;
    srcA.height = height;
    srcA.wstride = srcA.width;
    srcA.hstride = srcA.height;
    srcA.format = format;

    im_handle_param_t param;
    param.width = srcA.width;
    param.height = srcA.height;
    param.format = srcA.format;
    srcA.fd = -1;
    srcA.handle = importbuffer_virtualaddr((void *)inData, &param);

    srcB.width = width2;
    srcB.height = height2;
    srcB.wstride = srcB.width;
    srcB.hstride = srcB.height;
    srcB.format = format2;

    param.width = srcB.width;
    param.height = srcB.height;
    param.format = srcB.format;
    srcB.fd = -1;
    if(inData2){
        memcpy(pixels,inData2,width2*height2*3/2);
    }

    srcB.handle = importbuffer_virtualaddr((void *)pixels, &param);

    //imresize(srcA, srcB);

    usage |= IM_SYNC;

    srect.x = 0;
    srect.y = 0;
    srect.width = srcB.width;
    srect.height = srcB.height;

    drect.x = srcA.width-srcB.width;
    drect.y = srcA.height - srcB.height;
    drect.width = srcB.width;
    drect.height = srcB.height;

    ALOGE("srcB.width:%d,srcB.height:%d,srcB.wstride:%d,srcB.hstride:%d",
    srcB.width,srcB.height,srcB.wstride,srcB.hstride);

    ALOGE("drect.x:%d,drect.y:%d,drect.width:%d,drect.height:%d",
    drect.x,drect.y,drect.width,drect.height);

    IM_STATUS STATUS = improcess(  srcB,     srcA,    dst,
                srect,    drect,  prect,
                usage
                );
    ALOGE("improcess_ret:%s",imStrError(STATUS));

    releasebuffer_handle(srcA.handle);
    releasebuffer_handle(srcB.handle);
}


}  // namespace implementation
}  // namespace V3_4
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android
