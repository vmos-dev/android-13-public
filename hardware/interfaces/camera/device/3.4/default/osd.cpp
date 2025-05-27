#include "osd.h"

#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferAllocator.h>
#include <ui/GraphicBufferMapper.h>

#include <math.h>
#include <time.h>
#include <cwchar>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftglyph.h>

#include <png.h>
#include "pngstruct.h"
#include "pnginfo.h"


#include <inttypes.h>

#include "ExternalCameraDeviceSession_3.4.h"

#include "android-base/macros.h"
#include <utils/Timers.h>
#include <utils/Trace.h>
#include <linux/videodev2.h>
#include <sync/sync.h>

#define HAVE_JPEG // required for libyuv.h to export MJPEG decode APIs
#include <libyuv.h>

#include <jpeglib.h>
#include "RgaCropScale.h"
#include <RockchipRga.h>
#include <im2d_api/im2d.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferAllocator.h>
#include <ui/GraphicBufferMapper.h>
#ifndef RK_GRALLOC_4
#include "ExternalCameraGralloc.h"
#else
#include "ExternalCameraGralloc4.h"
#endif

#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_4 {
namespace implementation {

#define FONT_WIDTH      300
#define FONT_HEIGHT     30
#define WIDTH           50
#define HEIGHT          30
#define ALIGN(value, align) ((value + align -1) & ~(align-1))

int osd_time_pos_x,osd_time_pos_y;
int osd_logo_pos_x,osd_logo_pos_y;

int  OSD_IMAGE_W;
int  OSD_IMAGE_H;

FT_Library    mLibrary;
FT_Face       mFace;
FT_GlyphSlot  mSlot;
FT_Matrix     mMatrix;                 /* transformation matrix */
FT_Vector     mPen;                    /* untransformed origin  */

void deInitFt() {
	FT_Done_Face    (mFace);
	FT_Done_FreeType(mLibrary);
}

void resetFt() {
  mPen.x = 0 * 50;
  mPen.y = ( HEIGHT - 10 ) * 50;
}

int initFt() {

  FT_Error      error;

  char*         text;

  double        angle;
  int           target_height = HEIGHT;
  int           n;

  angle         = ( 0.0 / 360 ) * 3.14159 * 2;      /* use 25 degrees     */

  error = FT_Init_FreeType(&mLibrary);              /* initialize library */
  /* error handling omitted */

  error = FT_New_Face(mLibrary, FONT_FILE, 0, &mFace); /* create face object */
  /* error handling omitted */

#if 1
  /* use 50pt at 100dpi */
  error = FT_Set_Char_Size(mFace, 20 * 35, 0,
                            100, 0 );                /* set character size */

	/* pixels = 50 /72 * 100 = 69  */
#else
	FT_Set_Pixel_Sizes(mFace, 15, 0);
#endif
  /* error handling omitted */

  mSlot = mFace->glyph;

  /* set up matrix */
  mMatrix.xx = (FT_Fixed)( cos( angle ) * 0x10000L );
  mMatrix.xy = (FT_Fixed)(-sin( angle ) * 0x10000L );
  mMatrix.yx = (FT_Fixed)( sin( angle ) * 0x10000L );
  mMatrix.yy = (FT_Fixed)( cos( angle ) * 0x10000L );

  /* the pen position in 26.6 cartesian space coordinates; */
  /* start at (0,40) relative to the upper left corner  */
  mPen.x = 0 * 50;
  mPen.y = ( HEIGHT - 10 ) * 50;
  return 0;
}


void draw_bitmap( FT_Bitmap*  bitmap, unsigned char *buf,  FT_Int   x, FT_Int  y, int w, int h)
{
  FT_Int  i, j, p, q;
  FT_Int  x_max = x + bitmap->width;
  FT_Int  y_max = y + bitmap->rows;
  int pixByte = 4;
  for ( i = x, p = 0; i < x_max; i++, p++ )
  {
    for ( j = y, q = 0; j < y_max; j++, q++ )
    {
        unsigned char alth_vale = bitmap->buffer[q * bitmap->width + p];
        double alth =(alth_vale*1.0)/255;
        buf[(j*w+i)*pixByte]   = 0xff*alth;
        buf[(j*w+i)*pixByte+1] = 0xff*alth;
        buf[(j*w+i)*pixByte+2] = 0x00*alth;
        buf[(j*w+i)*pixByte+3] = alth_vale;
    }
  }
}
void wchar2RGBA(wchar_t  *text, unsigned char *rgba, int w, int h) {
        //FT_ENCODING_GB2312, FT_ENCODING_UNICODE
    int len = wcslen(text);
    FT_Select_Charmap(mFace, FT_ENCODING_UNICODE);
    FT_Set_Char_Size(mFace, 20 * 50, 0, 80, 0);
    for (int i=0; i<len; i++) {
       FT_Set_Transform(mFace, &mMatrix, &mPen);
       int result = FT_Load_Glyph(mFace, FT_Get_Char_Index(mFace, text[i]), FT_LOAD_DEFAULT);

       //FT_RENDER_MODE_MONO FT_RENDER_MODE_NORMAL 第二个参数为渲染模式
        result = FT_Render_Glyph(mFace->glyph,  FT_RENDER_MODE_NORMAL);
        FT_Bitmap bmp = mFace->glyph->bitmap;
        //int h = bmp.rows;
        //int w = bmp.width;
        draw_bitmap(&bmp, rgba,
                              mFace->glyph->bitmap_left,
                              HEIGHT - mFace->glyph->bitmap_top, w, h);
        mPen.x += mSlot->advance.x;
        mPen.y += mSlot->advance.y;
    }
}

void string2RGBA(char* text, unsigned char *rgba, int w, int h){

  int num_chars = strlen(text);
  FT_Error      error;
  for (int n = 0; n < num_chars; n++ )
  {
    /* set transformation */
    FT_Set_Transform(mFace, &mMatrix, &mPen);

    /* load glyph image into the slot (erase previous one) */
    error = FT_Load_Char(mFace, text[n], FT_LOAD_RENDER );
    if ( error )
      continue;                 /* ignore errors */
    /* now, draw to our target surface (convert position) */
    draw_bitmap( &mSlot->bitmap, rgba,
                  mSlot->bitmap_left,
                 HEIGHT- mSlot->bitmap_top, w, h);

    /* increment pen position */
    mPen.x += mSlot->advance.x;
    mPen.y += mSlot->advance.y;
  }
}

void getCSTTimeFormat(char* pStdTimeFormat)
{
	time_t nTimeStamp;
	time(&nTimeStamp);
	char pTmpString[256] = {0};
	tm *pTmStruct = localtime(&nTimeStamp);
	sprintf(pTmpString, "%04d-%02d-%02d %02d:%02d:%02d", pTmStruct->tm_year + 1900, pTmStruct->tm_mon + 1, pTmStruct->tm_mday, \
		pTmStruct->tm_hour, pTmStruct->tm_min, pTmStruct->tm_sec);
	strcpy(pStdTimeFormat, pTmpString);
}

void getCSTTimeFormatUnicode(wchar_t* pStdTimeFormat)
{
        time_t  nTimeStamp;
        time(&nTimeStamp);
        wchar_t pTmpString[256] = {0};
        tm *pTmStruct = localtime(&nTimeStamp);
        swprintf(pTmpString, 256, OSD_TEXT, pTmStruct->tm_year + 1900, pTmStruct->tm_mon + 1, pTmStruct->tm_mday, \
                pTmStruct->tm_hour, pTmStruct->tm_min, pTmStruct->tm_sec);
        wmemcpy(pStdTimeFormat, pTmpString, 128);
        return;
}

void getGMTTimeFormat(char* pStdTimeFormat)
{
	time_t ltime;
	time(&ltime);
	//ltime += 8*3600; //北京时区
	tm* gmt = gmtime(&ltime);
	char s[128] = { 0 };
	strftime(s, 80, "%Y-%m-%d %H:%M:%S", gmt);
	strcpy(pStdTimeFormat, s);
}

void timeFormat2Timestamp(const char* strTimeFormat, time_t& timeStamp)
{
  // strTimeFormat should be such as "2001-11-12 18:31:01"
  struct tm *timeinfo;
  memset( timeinfo, 0, sizeof(struct tm));
  strptime(strTimeFormat, "%Y-%m-%d %H:%M:%S",  timeinfo);
  timeStamp = mktime(timeinfo);
}

int DecodePNG(char* png_path,unsigned int* buf) {
  FILE *fp;
  fp = fopen(png_path, "rb");
  png_byte sig[8];
  fread(sig, 1, 8, fp);
  if(png_sig_cmp(sig, 0, 8)){
    fclose(fp);
    return 0;
  }

  png_structp png_ptr;
  png_infop info_ptr;

  png_ptr = png_create_read_struct(png_get_libpng_ver(NULL), NULL, NULL, NULL);
  info_ptr = png_create_info_struct(png_ptr);
  setjmp(png_jmpbuf(png_ptr));
  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);
  png_read_info(png_ptr, info_ptr);

  ALOGD("PNG INFO(%s):\n pixel_depth = %d,bit_depth = %d, width = %d,height = %d", png_path,
  info_ptr->pixel_depth,info_ptr->bit_depth,
  info_ptr->width,info_ptr->height);

  png_bytep row_pointers[info_ptr->height];
  int row;
  for (row = 0; row < info_ptr->height; row++){
    row_pointers[row] = NULL;
    row_pointers[row] = (png_bytep)png_malloc(png_ptr, png_get_rowbytes(png_ptr,info_ptr));
  }
  png_read_image(png_ptr, row_pointers);
  int i, j;
  OSD_IMAGE_W = ALIGN(info_ptr->width,16);
  OSD_IMAGE_H = info_ptr->height;
  info_ptr->width = OSD_IMAGE_W;
  unsigned int *pDst = buf;//(unsigned int*)malloc(size);//因为sizeof(unsigned long)=8

  printf("sizeof(unsigned int) = %d\n", (int)sizeof(unsigned int));
  if(info_ptr->pixel_depth == 32){
      unsigned char* pSrc;
      unsigned int pixelR,pixelG,pixelB,pixelA;
      for(j=0; j<info_ptr->height; j++){
          pSrc = row_pointers[j];
          for(i=0; i<info_ptr->width; i++){
              pixelR = *pSrc++;
              pixelG = *pSrc++;
              pixelB = *pSrc++;
              pixelA = *pSrc++;
              pDst[i] = (pixelR<< 24) | (pixelR << 16) | (pixelB << 8) | pixelA;
              if (pixelA==0) {
                pDst[i] = 0;
              }
          }
          pDst += info_ptr->width;
      }
  }

  for (row = 0; row < info_ptr->height; row++){
    png_free(png_ptr,row_pointers[row]);
  }
  fclose(fp);
  png_free_data(png_ptr,info_ptr,PNG_FREE_ALL,-1);
  png_destroy_read_struct(&png_ptr,NULL,NULL);
  return 0;
}

void processOSD(int width,int height,unsigned long dst_fd){
  static uint32_t *pixelsLogo = nullptr;
  if(pixelsLogo == nullptr){
    static buffer_handle_t memHandle = nullptr;
    android::GraphicBufferAllocator& alloc(android::GraphicBufferAllocator::get());
    const auto usage =
        GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_SW_READ_RARELY | GRALLOC_USAGE_SW_WRITE_OFTEN;
    unsigned pixelsPerLine;
    android::status_t result =
            alloc.allocate(500, 500, HAL_PIXEL_FORMAT_RGBA_8888, 1, usage, &memHandle, &pixelsPerLine, 0,
                            "ExternalCameraDevice");
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    mapper.lock(memHandle,
                GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_SW_READ_NEVER,
                android::Rect(500, 500),
                (void **) &pixelsLogo);

    // If we failed to lock the pixel buffer, we're about to crash, but log it first
    if (!pixelsLogo) {
        ALOGE("Camera failed to gain access to image buffer for writing");
    }
    DecodePNG(PNG_LOGO,pixelsLogo);
  }

  static uint32_t *pixelsFont = nullptr;
  if(pixelsFont == nullptr){
    static buffer_handle_t memHandle = nullptr;
    android::GraphicBufferAllocator& alloc(android::GraphicBufferAllocator::get());
    const auto usage =
        GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_SW_READ_RARELY | GRALLOC_USAGE_SW_WRITE_OFTEN;
    unsigned pixelsPerLine;
    android::status_t result =
            alloc.allocate(500, 500, HAL_PIXEL_FORMAT_RGBA_8888, 1, usage, &memHandle, &pixelsPerLine, 0,
                            "ExternalCameraDevice");
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    mapper.lock(memHandle,
                GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_SW_READ_NEVER,
                android::Rect(500, 500),
                (void **) &pixelsFont);

    // If we failed to lock the pixel buffer, we're about to crash, but log it first
    if (!pixelsFont) {
        ALOGE("Camera failed to gain access to image buffer for writing");
    }
    initFt();

    wchar_t pStdTimeFormat[128] = { 0 };
    getCSTTimeFormatUnicode(pStdTimeFormat);
    memset((unsigned char*)pixelsFont,0x00,FONT_WIDTH*FONT_HEIGHT*4);

    wchar2RGBA(pStdTimeFormat, (unsigned char*)pixelsFont, FONT_WIDTH, FONT_HEIGHT);
    resetFt();
    deInitFt();
  }

	camera2::RgaCropScale::Params rgain, rgain0, rgaout;
	unsigned char* timeOsdVddr = NULL;
	unsigned char* labelOsdVddr = NULL;
	int timeOsdFd = -1;
	int labelOsdFd = -1;
	int timeOsdWidth = 0;
	int timeOsdHeight = 0;
	int labelOsdWidth = 0;
	int labelOsdHeight = 0;

  timeOsdVddr = (unsigned char*)pixelsFont;

  labelOsdVddr = (unsigned char*)pixelsLogo;
  timeOsdWidth = FONT_WIDTH;
  timeOsdHeight = FONT_HEIGHT;
  labelOsdWidth = OSD_IMAGE_W;
  labelOsdHeight = OSD_IMAGE_H;

  osd_time_pos_x = width - timeOsdWidth;
  osd_time_pos_y = height - timeOsdHeight;
  ALOGD("osd_time_pos_x:%d,osd_time_pos_y:%d",osd_time_pos_x,osd_time_pos_y);

  rgain.fd = timeOsdFd;
	rgain.fmt = HAL_PIXEL_FORMAT_RGBA_8888;
	rgain.vir_addr = (char*)timeOsdVddr;
	rgain.mirror = false;
	rgain.width = timeOsdWidth;
	rgain.height = timeOsdHeight;
	rgain.offset_x = 0;
	rgain.offset_y = 0;
	rgain.width_stride = timeOsdWidth;
	rgain.height_stride = timeOsdHeight;
	rgain.translate_x = osd_time_pos_x;
	rgain.translate_y = osd_time_pos_y;
	rgain.blend = 1;

	rgaout.fd = dst_fd;
	rgaout.fmt = HAL_PIXEL_FORMAT_YCrCb_NV12;
	rgaout.mirror = false;
	rgaout.width = width;
	rgaout.height =  height;
	rgaout.offset_x = 0;
	rgaout.offset_y = 0;
	rgaout.width_stride = width;
	rgaout.height_stride = height;
  camera2::RgaCropScale::Im2dBlit(&rgain, &rgaout);

  ALOGD("labelOsdWidth:%d,labelOsdHeight:%d",labelOsdWidth,labelOsdHeight);
  osd_logo_pos_x = width - labelOsdWidth;
  osd_logo_pos_y = height - labelOsdHeight - timeOsdHeight;
  ALOGD("osd_logo_pos_x:%d,osd_logo_pos_y:%d",osd_logo_pos_x,osd_logo_pos_y);

  rgain0.fd = labelOsdFd;
	rgain0.fmt = HAL_PIXEL_FORMAT_RGBA_8888;
	rgain0.vir_addr = (char*)labelOsdVddr;
	rgain0.mirror = false;
	rgain0.width = labelOsdWidth;
	rgain0.height = labelOsdHeight;
	rgain0.offset_x = 0;
	rgain0.offset_y = 0;
	rgain0.width_stride = labelOsdWidth;
	rgain0.height_stride = labelOsdHeight;
	rgain0.translate_x = osd_logo_pos_x;
	rgain0.translate_y = osd_logo_pos_y;
	rgain0.blend = 1;

  camera2::RgaCropScale::Im2dBlit(&rgain0, &rgaout);
}


}  // namespace implementation
}  // namespace V3_4
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android
