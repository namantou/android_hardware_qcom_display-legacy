/*
* Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*    * Redistributions of source code must retain the above copyright
*      notice, this list of conditions and the following disclaimer.
*    * Redistributions in binary form must reproduce the above
*      copyright notice, this list of conditions and the following
*      disclaimer in the documentation and/or other materials provided
*      with the distribution.
*    * Neither the name of Code Aurora Forum, Inc. nor the names of its
*      contributors may be used to endorse or promote products derived
*      from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef OVERLAY_UTILS_H
#define OVERLAY_UTILS_H

#include <cutils/log.h> // ALOGE, etc
#include <errno.h>
#include <fcntl.h> // open, O_RDWR, etc
#include <hardware/hardware.h>
#include <hardware/gralloc.h> // buffer_handle_t
#include <linux/msm_mdp.h> // flags
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utils/Log.h>
#include <mdp_version.h>
#include "gralloc_priv.h" //for interlace

/*
*
* Collection of utilities functions/structs/enums etc...
*
* */

// comment that out if you want to remove asserts
// or put it as -D in Android.mk. your choice.
#define OVERLAY_HAS_ASSERT

#ifdef OVERLAY_HAS_ASSERT
# define OVASSERT(x, ...) if(!(x)) { ALOGE(__VA_ARGS__); abort(); }
#else
# define OVASSERT(x, ...) ALOGE_IF(!(x), __VA_ARGS__)
#endif // OVERLAY_HAS_ASSERT

#define DEBUG_OVERLAY 0
#define PROFILE_OVERLAY 0

namespace overlay {

// fwd
class Overlay;
class OvFD;

/* helper function to open by using fbnum */
bool open(OvFD& fd, uint32_t fbnum, const char* const dev,
    int flags = O_RDWR);

namespace utils {
struct Whf;
struct Dim;

inline uint32_t setBit(uint32_t x, uint32_t mask) {
    return (x | mask);
}

inline uint32_t clrBit(uint32_t x, uint32_t mask) {
    return (x & ~mask);
}

/* Utility class to help avoid copying instances by making the copy ctor
* and assignment operator private
*
* Usage:
*    class SomeClass : utils::NoCopy {...};
*/
class NoCopy {
protected:
    NoCopy(){}
    ~NoCopy() {}
private:
    NoCopy(const NoCopy&);
    const NoCopy& operator=(const NoCopy&);
};

/*
* Utility class to query the framebuffer info for primary display
*
* Usage:
*    Outside of functions:
*       utils::FrameBufferInfo* utils::FrameBufferInfo::sFBInfoInstance = 0;
*    Inside functions:
*       utils::FrameBufferInfo::getInstance()->supportTrueMirroring()
*/
class FrameBufferInfo {

public:
    /* ctor init */
    explicit FrameBufferInfo();

    /* Gets an instance if one does not already exist */
    static FrameBufferInfo* getInstance();

    /* Gets width of primary framebuffer */
    int getWidth() const;

    /* Gets height of primary framebuffer */
    int getHeight() const;

private:
    int mFBWidth;
    int mFBHeight;
    bool mBorderFillSupported;
    static FrameBufferInfo *sFBInfoInstance;
};

/* 3D related utils, defines etc...
 * The compound format passed to the overlay is
 * ABCCC where A is the input 3D format
 * B is the output 3D format
 * CCC is the color format e.g YCbCr420SP YCrCb420SP etc */
enum { SHIFT_OUT_3D = 12,
    SHIFT_TOT_3D = 16 };
enum { INPUT_3D_MASK = 0xFFFF0000,
    OUTPUT_3D_MASK = 0x0000FFFF };
enum { BARRIER_LAND = 1,
    BARRIER_PORT = 2 };

/* if SurfaceFlinger process gets killed in bypass mode, In initOverlay()
 * close all the pipes if it is opened after reboot.
 */
int initOverlay(void);

inline uint32_t format3D(uint32_t x) { return x & 0xFF000; }
inline uint32_t colorFormat(uint32_t fmt) {
    /*TODO enable this block only if format has interlace / 3D info in top bits.
    if(fmt & INTERLACE_MASK) {
        fmt = fmt ^ HAL_PIXEL_FORMAT_INTERLACE;
    }
    fmt = fmt & 0xFFF;*/
    return fmt;
}
inline uint32_t format3DOutput(uint32_t x) {
    return (x & 0xF000) >> SHIFT_OUT_3D; }
inline uint32_t format3DInput(uint32_t x) { return x & 0xF0000; }
uint32_t getColorFormat(uint32_t format);

bool isHDMIConnected ();
bool is3DTV();
bool isPanel3D();
bool usePanel3D();
bool send3DInfoPacket (uint32_t fmt);
bool enableBarrier (uint32_t orientation);
uint32_t getS3DFormat(uint32_t fmt);

template <int CHAN>
bool getPositionS3D(const Whf& whf, Dim& out);

template <int CHAN>
bool getCropS3D(const Dim& in, Dim& out, uint32_t fmt);

template <class Type>
void swapWidthHeight(Type& width, Type& height);

struct Dim {
    Dim () : x(0), y(0),
    w(0), h(0),
    o(0) {}
    Dim(uint32_t _x, uint32_t _y, uint32_t _w, uint32_t _h) :
        x(_x), y(_y),
        w(_w), h(_h) {}
    Dim(uint32_t _x, uint32_t _y, uint32_t _w, uint32_t _h, uint32_t _o) :
        x(_x), y(_y),
        w(_w), h(_h),
        o(_o) {}
    bool check(uint32_t _w, uint32_t _h) const {
        return (x+w <= _w && y+h <= _h);

    }

    bool operator==(const Dim& d) const {
        return d.x == x && d.y == y &&
                d.w == w && d.h == h &&
                d.o == o;
    }

    bool operator!=(const Dim& d) const {
        return !operator==(d);
    }

    void dump() const;
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
    uint32_t o;
};

// TODO have Whfz

struct Whf {
    Whf() : w(0), h(0), format(0), size(0) {}
    Whf(uint32_t wi, uint32_t he, uint32_t f) :
        w(wi), h(he), format(f), size(0) {}
    Whf(uint32_t wi, uint32_t he, uint32_t f, uint32_t s) :
        w(wi), h(he), format(f), size(s) {}
    // FIXME not comparing size at the moment
    bool operator==(const Whf& whf) const {
        return whf.w == w && whf.h == h &&
                whf.format == format;
    }
    bool operator!=(const Whf& whf) const {
        return !operator==(whf);
    }
    void dump() const;
    uint32_t w;
    uint32_t h;
    uint32_t format;
    uint32_t size;
};

class ActionSafe {
private:
    ActionSafe() : mWidth(0.0f), mHeight(0.0f) { };
    float mWidth;
    float mHeight;
    static ActionSafe *sActionSafe;
public:
    ~ActionSafe() { };
    static ActionSafe* getInstance() {
        if(!sActionSafe) {
            sActionSafe = new ActionSafe();
        }
        return sActionSafe;
    }
    void setDimension(int w, int h) {
        mWidth = (float)w;
        mHeight = (float)h;
    }
    float getWidth() { return mWidth; }
    float getHeight() { return mHeight; }
};

enum { MAX_PATH_LEN = 256 };

/**
 * Rotator flags: not to be confused with orientation flags.
 * Ususally, you want to open the rotator to make sure it is
 * ready for business.
 * ROT_FLAG_DISABLED: Rotator not used unless required.
 * ROT_FLAG_ENABLED: Rotator used even if not required.
 * */
enum eRotFlags {
    ROT_FLAG_DISABLED = 0,
    ROT_FLAG_ENABLED = 1 // needed in rot
};

/* The values for is_fg flag for control alpha and transp
 * IS_FG_OFF means is_fg = 0
 * IS_FG_SET means is_fg = 1
 */
enum eIsFg {
    IS_FG_OFF = 0,
    IS_FG_SET = 1
};

/*
 * Various mdp flags like PIPE SHARE, DEINTERLACE etc...
 * kernel/common/linux/msm_mdp.h
 * INTERLACE_MASK: hardware/qcom/display/libgralloc/badger/fb_priv.h
 * */
enum eMdpFlags {
    OV_MDP_FLAGS_NONE = 0,
    OV_MDP_PIPE_SHARE =  MDP_OV_PIPE_SHARE,
    OV_MDP_DEINTERLACE = MDP_DEINTERLACE,
    OV_MDP_SECURE_OVERLAY_SESSION = MDP_SECURE_OVERLAY_SESSION,
    OV_MDP_SOURCE_ROTATED_90 = MDP_SOURCE_ROTATED_90,
    OV_MDP_MEMORY_ID_TYPE_FB = MDP_MEMORY_ID_TYPE_FB,
    OV_MDP_BACKEND_COMPOSITION = MDP_BACKEND_COMPOSITION,
    OV_MDP_BLEND_FG_PREMULT = MDP_BLEND_FG_PREMULT,
    OV_MDP_FLIP_H = MDP_FLIP_LR,
    OV_MDP_FLIP_V = MDP_FLIP_UD,
};

enum eZorder {
    ZORDER_0,
    ZORDER_1,
    ZORDER_2,
    ZORDER_3,
    Z_SYSTEM_ALLOC = 0xFFFF
};

enum eMdpPipeType {
    OV_MDP_PIPE_RGB,
    OV_MDP_PIPE_VG,
    OV_MDP_PIPE_ANY, //Any
};

/* Used to identify destination pipes
 */
enum eDest {
    OV_VG0 = 0,
    OV_RGB0,
    OV_VG1,
    OV_RGB1,
    OV_VG2,
    OV_RGB2,
    OV_INVALID,
};

/* Used when a buffer is split over 2 pipes and sent to display */
enum {
    OV_LEFT_SPLIT = 0,
    OV_RIGHT_SPLIT,
};

/* values for copybit_set_parameter(OVERLAY_TRANSFORM) */
enum eTransform {
    /* No rot */
    OVERLAY_TRANSFORM_0 = 0x0,
    /* flip source image horizontally 0x1 */
    OVERLAY_TRANSFORM_FLIP_H = HAL_TRANSFORM_FLIP_H,
    /* flip source image vertically 0x2 */
    OVERLAY_TRANSFORM_FLIP_V = HAL_TRANSFORM_FLIP_V,
    /* rotate source image 180 degrees
     * It is basically bit-or-ed  H | V == 0x3 */
    OVERLAY_TRANSFORM_ROT_180 = HAL_TRANSFORM_ROT_180,
    /* rotate source image 90 degrees 0x4 */
    OVERLAY_TRANSFORM_ROT_90 = HAL_TRANSFORM_ROT_90,
    /* rotate source image 90 degrees and flip horizontally 0x5 */
    OVERLAY_TRANSFORM_ROT_90_FLIP_H = HAL_TRANSFORM_ROT_90 |
                                      HAL_TRANSFORM_FLIP_H,
    /* rotate source image 90 degrees and flip vertically 0x6 */
    OVERLAY_TRANSFORM_ROT_90_FLIP_V = HAL_TRANSFORM_ROT_90 |
                                      HAL_TRANSFORM_FLIP_V,
    /* rotate source image 270 degrees
     * Basically 180 | 90 == 0x7 */
    OVERLAY_TRANSFORM_ROT_270 = HAL_TRANSFORM_ROT_270,
    /* rotate invalid like in Transform.h */
    OVERLAY_TRANSFORM_INV = 0x80
};

// Used to consolidate pipe params
struct PipeArgs {
    PipeArgs() : mdpFlags(OV_MDP_FLAGS_NONE),
        zorder(Z_SYSTEM_ALLOC),
        isFg(IS_FG_OFF),
        rotFlags(ROT_FLAG_DISABLED){
    }

    PipeArgs(eMdpFlags f, Whf _whf,
            eZorder z, eIsFg fg, eRotFlags r) :
        mdpFlags(f),
        whf(_whf),
        zorder(z),
        isFg(fg),
        rotFlags(r) {
    }

    eMdpFlags mdpFlags; // for mdp_overlay flags
    Whf whf;
    eZorder zorder; // stage number
    eIsFg isFg; // control alpha & transp
    eRotFlags rotFlags;
};

inline void setMdpFlags(eMdpFlags& f, eMdpFlags v) {
    f = static_cast<eMdpFlags>(setBit(f, v));
}

inline void clearMdpFlags(eMdpFlags& f, eMdpFlags v) {
    f = static_cast<eMdpFlags>(clrBit(f, v));
}

// fb 0/1/2
enum { FB0, FB1, FB2 };

//Panels could be categorized as primary and external
enum { PRIMARY, EXTERNAL };

// 2 for rgb0/1 double bufs
enum { RGB_PIPE_NUM_BUFS = 2 };

struct ScreenInfo {
    ScreenInfo() : mFBWidth(0),
    mFBHeight(0),
    mFBbpp(0),
    mFBystride(0) {}
    void dump(const char* const s) const;
    uint32_t mFBWidth;
    uint32_t mFBHeight;
    uint32_t mFBbpp;
    uint32_t mFBystride;
};

int getMdpFormat(int format);
int getRotOutFmt(uint32_t format);
/* flip is upside down and such. V, H flip
 * rotation is 90, 180 etc
 * It returns MDP related enum/define that match rot+flip*/
int getMdpOrient(eTransform rotation);
int getOverlayMagnificationLimit();
const char* getFormatString(int format);

enum {
    HW_OV_MINIFICATION_LIMIT  = 8
};

template <class T>
inline void memset0(T& t) { ::memset(&t, 0, sizeof(T)); }

template <class T> inline void swap ( T& a, T& b )
{
    T c(a); a=b; b=c;
}

inline int alignup(int value, int a) {
    //if align = 0, return the value. Else, do alignment.
    return a ? ((((value - 1) / a) + 1) * a) : value;
}

// FIXME that align should replace the upper one.
inline int align(int value, int a) {
    //if align = 0, return the value. Else, do alignment.
    return a ? ((value + (a-1)) & ~(a-1)) : value;
}

enum eRotOutFmt {
    ROT_OUT_FMT_DEFAULT,
    ROT_OUT_FMT_Y_CRCB_H2V2
};

template <int ROT_OUT_FMT> struct RotOutFmt;

// FIXME, taken from gralloc_priv.h. Need to
// put it back as soon as overlay takes place of the old one
/* possible formats for 3D content*/
enum {
    HAL_NO_3D                         = 0x0000,
    HAL_3D_IN_SIDE_BY_SIDE_L_R        = 0x10000,
    HAL_3D_IN_TOP_BOTTOM              = 0x20000,
    HAL_3D_IN_INTERLEAVE              = 0x40000,
    HAL_3D_IN_SIDE_BY_SIDE_R_L        = 0x80000,
    HAL_3D_OUT_SIDE_BY_SIDE           = 0x1000,
    HAL_3D_OUT_TOP_BOTTOM             = 0x2000,
    HAL_3D_OUT_INTERLEAVE             = 0x4000,
    HAL_3D_OUT_MONOSCOPIC             = 0x8000
};

enum { HAL_3D_OUT_SBS_MASK =
    HAL_3D_OUT_SIDE_BY_SIDE >> overlay::utils::SHIFT_OUT_3D,
    HAL_3D_OUT_TOP_BOT_MASK =
            HAL_3D_OUT_TOP_BOTTOM >> overlay::utils::SHIFT_OUT_3D,
    HAL_3D_OUT_INTERL_MASK =
            HAL_3D_OUT_INTERLEAVE >> overlay::utils::SHIFT_OUT_3D,
    HAL_3D_OUT_MONOS_MASK =
            HAL_3D_OUT_MONOSCOPIC >> overlay::utils::SHIFT_OUT_3D
};


inline bool isYuv(uint32_t format) {
    switch(format){
        case MDP_Y_CBCR_H2V1:
        case MDP_Y_CBCR_H2V2:
        case MDP_Y_CRCB_H2V2:
        case MDP_Y_CRCB_H1V1:
        case MDP_Y_CRCB_H2V1:
        case MDP_Y_CRCB_H2V2_TILE:
        case MDP_Y_CBCR_H2V2_TILE:
        case MDP_Y_CR_CB_H2V2:
        case MDP_Y_CR_CB_GH2V2:
            return true;
        default:
            return false;
    }
    return false;
}

inline bool isRgb(uint32_t format) {
    switch(format) {
        case MDP_RGBA_8888:
        case MDP_BGRA_8888:
        case MDP_RGBX_8888:
        case MDP_RGB_565:
            return true;
        default:
            return false;
    }
    return false;
}

inline const char* getFormatString(int format){
    static const char* const formats[] = {
        "MDP_RGB_565",
        "MDP_XRGB_8888",
        "MDP_Y_CBCR_H2V2",
        "MDP_Y_CBCR_H2V2_ADRENO",
        "MDP_ARGB_8888",
        "MDP_RGB_888",
        "MDP_Y_CRCB_H2V2",
        "MDP_YCRYCB_H2V1",
        "MDP_Y_CRCB_H2V1",
        "MDP_Y_CBCR_H2V1",
        "MDP_Y_CRCB_H1V2",
        "MDP_Y_CBCR_H1V2",
        "MDP_RGBA_8888",
        "MDP_BGRA_8888",
        "MDP_RGBX_8888",
        "MDP_Y_CRCB_H2V2_TILE",
        "MDP_Y_CBCR_H2V2_TILE",
        "MDP_Y_CR_CB_H2V2",
        "MDP_Y_CR_CB_GH2V2",
        "MDP_Y_CB_CR_H2V2",
        "MDP_Y_CRCB_H1V1",
        "MDP_Y_CBCR_H1V1",
        "MDP_YCRCB_H1V1",
        "MDP_YCBCR_H1V1",
        "MDP_BGR_565",
        "MDP_IMGTYPE_LIMIT",
        "MDP_RGB_BORDERFILL",
        "MDP_FB_FORMAT",
        "MDP_IMGTYPE_LIMIT2"
    };
    if(format < 0 || format >= (int)(sizeof(formats) / sizeof(formats[0]))) {
        ALOGE("%s wrong fmt %d", __FUNCTION__, format);
        return "Unsupported format";
    }
    return formats[format];
}

inline void Whf::dump() const {
    ALOGE("== Dump WHF w=%d h=%d f=%d s=%d start/end ==",
            w, h, format, size);
}

inline void Dim::dump() const {
    ALOGE("== Dump Dim x=%d y=%d w=%d h=%d start/end ==", x, y, w, h);
}

inline int getMdpOrient(eTransform rotation) {
    ALOGE_IF(DEBUG_OVERLAY, "%s: rot=%d", __FUNCTION__, rotation);
    switch(rotation)
    {
        case OVERLAY_TRANSFORM_0 : return 0;
        case OVERLAY_TRANSFORM_FLIP_V:  return MDP_FLIP_UD;
        case OVERLAY_TRANSFORM_FLIP_H:  return MDP_FLIP_LR;
        case OVERLAY_TRANSFORM_ROT_90:  return MDP_ROT_90;
        //getMdpOrient will switch the flips if the source is 90 rotated.
        //Clients in Android dont factor in 90 rotation while deciding flip.
        case OVERLAY_TRANSFORM_ROT_90_FLIP_V:
                return MDP_ROT_90 | MDP_FLIP_LR;
        case OVERLAY_TRANSFORM_ROT_90_FLIP_H:
                return MDP_ROT_90 | MDP_FLIP_UD;
        case OVERLAY_TRANSFORM_ROT_180: return MDP_ROT_180;
        case OVERLAY_TRANSFORM_ROT_270: return MDP_ROT_270;
        default:
            ALOGE("%s: invalid rotation value (value = 0x%x",
                    __FUNCTION__, rotation);
    }
    return -1;
}

inline int getRotOutFmt(uint32_t format) {
    switch (format) {
        case MDP_Y_CRCB_H2V2_TILE:
            return MDP_Y_CRCB_H2V2;
        case MDP_Y_CBCR_H2V2_TILE:
            return MDP_Y_CBCR_H2V2;
        case MDP_Y_CB_CR_H2V2:
            return MDP_Y_CBCR_H2V2;
        case MDP_Y_CR_CB_GH2V2:
            return MDP_Y_CRCB_H2V2;
        default:
            return format;
    }
    // not reached
    OVASSERT(false, "%s not reached", __FUNCTION__);
    return -1;
}


inline uint32_t getColorFormat(uint32_t format)
{
    return (format == HAL_PIXEL_FORMAT_YV12) ?
            format : colorFormat(format);
}

// FB0
template <int CHAN>
inline Dim getPositionS3DImpl(const Whf& whf)
{
    switch (whf.format & OUTPUT_3D_MASK)
    {
        case HAL_3D_OUT_SBS_MASK:
            // x, y, w, h
            return Dim(0, 0, whf.w/2, whf.h);
        case HAL_3D_OUT_TOP_BOT_MASK:
            return Dim(0, 0, whf.w, whf.h/2);
        case HAL_3D_OUT_MONOS_MASK:
            return Dim();
        case HAL_3D_OUT_INTERL_MASK:
            // FIXME error?
            ALOGE("%s HAL_3D_OUT_INTERLEAVE_MASK", __FUNCTION__);
            return Dim();
        default:
            ALOGE("%s Unsupported 3D output format %d", __FUNCTION__,
                    whf.format);
    }
    return Dim();
}

template <>
inline Dim getPositionS3DImpl<utils::OV_RIGHT_SPLIT>(const Whf& whf)
{
    switch (whf.format & OUTPUT_3D_MASK)
    {
        case HAL_3D_OUT_SBS_MASK:
            return Dim(whf.w/2, 0, whf.w/2, whf.h);
        case HAL_3D_OUT_TOP_BOT_MASK:
            return Dim(0, whf.h/2, whf.w, whf.h/2);
        case HAL_3D_OUT_MONOS_MASK:
            return Dim(0, 0, whf.w, whf.h);
        case HAL_3D_OUT_INTERL_MASK:
            // FIXME error?
            ALOGE("%s HAL_3D_OUT_INTERLEAVE_MASK", __FUNCTION__);
            return Dim();
        default:
            ALOGE("%s Unsupported 3D output format %d", __FUNCTION__,
                    whf.format);
    }
    return Dim();
}

template <int CHAN>
inline bool getPositionS3D(const Whf& whf, Dim& out) {
    out = getPositionS3DImpl<CHAN>(whf);
    return (out != Dim());
}

template <int CHAN>
inline Dim getCropS3DImpl(const Dim& in, uint32_t fmt) {
    switch (fmt & INPUT_3D_MASK)
    {
        case HAL_3D_IN_SIDE_BY_SIDE_L_R:
            return Dim(0, 0, in.w/2, in.h);
        case HAL_3D_IN_SIDE_BY_SIDE_R_L:
            return Dim(in.w/2, 0, in.w/2, in.h);
        case HAL_3D_IN_TOP_BOTTOM:
            return Dim(0, 0, in.w, in.h/2);
        case HAL_3D_IN_INTERLEAVE:
            ALOGE("%s HAL_3D_IN_INTERLEAVE", __FUNCTION__);
            break;
        default:
            ALOGE("%s Unsupported 3D format %d", __FUNCTION__, fmt);
            break;
    }
    return Dim();
}

template <>
inline Dim getCropS3DImpl<utils::OV_RIGHT_SPLIT>(const Dim& in, uint32_t fmt) {
    switch (fmt & INPUT_3D_MASK)
    {
        case HAL_3D_IN_SIDE_BY_SIDE_L_R:
            return Dim(in.w/2, 0, in.w/2, in.h);
        case HAL_3D_IN_SIDE_BY_SIDE_R_L:
            return Dim(0, 0, in.w/2, in.h);
        case HAL_3D_IN_TOP_BOTTOM:
            return Dim(0, in.h/2, in.w, in.h/2);
        case HAL_3D_IN_INTERLEAVE:
            ALOGE("%s HAL_3D_IN_INTERLEAVE", __FUNCTION__);
            break;
        default:
            ALOGE("%s Unsupported 3D format %d", __FUNCTION__, fmt);
            break;
    }
    return Dim();
}

template <int CHAN>
inline bool getCropS3D(const Dim& in, Dim& out, uint32_t fmt)
{
    out = getCropS3DImpl<CHAN>(in, fmt);
    return (out != Dim());
}

template <class Type>
void swapWidthHeight(Type& width, Type& height) {
    Type tmp = width;
    width = height;
    height = tmp;
}

inline void ScreenInfo::dump(const char* const s) const {
    ALOGE("== Dump %s ScreenInfo w=%d h=%d"
            " bpp=%d stride=%d start/end ==",
            s, mFBWidth, mFBHeight, mFBbpp, mFBystride);
}

inline bool openDev(OvFD& fd, int fbnum,
    const char* const devpath, int flags) {
    return overlay::open(fd, fbnum, devpath, flags);
}

template <class T>
inline void even_ceil(T& value) {
    if(value & 1)
        value++;
}

template <class T>
inline void even_floor(T& value) {
    if(value & 1)
        value--;
}

inline const char* getDestStr(eDest dest) {
    switch(dest) {
        case OV_VG0: return "VG0";
        case OV_RGB0: return "RGB0";
        case OV_VG1: return "VG1";
        case OV_RGB1: return "RGB1";
        case OV_VG2: return "VG2";
        case OV_RGB2: return "RGB2";
        default: return "Invalid";
    }
    return "Invalid";
}

inline eMdpPipeType getPipeType(eDest dest) {
    switch(dest) {
        case OV_VG0:
        case OV_VG1:
        case OV_VG2:
            return OV_MDP_PIPE_VG;
        case OV_RGB0:
        case OV_RGB1:
        case OV_RGB2:
            return OV_MDP_PIPE_RGB;
        default:
            return OV_MDP_PIPE_ANY;
    }
    return OV_MDP_PIPE_ANY;
}

} // namespace utils ends

//--------------------Class Res stuff (namespace overlay only) -----------

class Res {
public:
    // /dev/graphics/fb%u
    static const char* const fbPath;
    // /dev/msm_rotator
    static const char* const rotPath;
    // /sys/class/graphics/fb1/format_3d
    static const char* const format3DFile;
    // /sys/class/graphics/fb1/3d_present
    static const char* const edid3dInfoFile;
    // /sys/devices/platform/mipi_novatek.0/enable_3d_barrier
    static const char* const barrierFile;
};


//--------------------Class OvFD stuff (namespace overlay only) -----------

/*
* Holds one FD
* Dtor will NOT close the underlying FD.
* That enables us to copy that object around
* */
class OvFD {
public:
    /* Ctor */
    explicit OvFD();

    /* dtor will NOT close the underlying FD */
    ~OvFD();

    /* Open fd using the path given by dev.
     * return false in failure */
    bool open(const char* const dev,
            int flags = O_RDWR);

    /* populate path */
    void setPath(const char* const dev);

    /* Close fd if we have a valid fd. */
    bool close();

    /* returns underlying fd.*/
    int getFD() const;

    /* returns true if fd is valid */
    bool valid() const;

    /* like operator= */
    void copy(int fd);

    /* dump the state of the instance */
    void dump() const;
private:
    /* helper enum for determine valid/invalid fd */
    enum { INVAL = -1 };

    /* actual os fd */
    int mFD;

    /* path, for debugging */
    char mPath[utils::MAX_PATH_LEN];
};

//-------------------Inlines--------------------------

inline bool open(OvFD& fd, uint32_t fbnum, const char* const dev, int flags)
{
    char dev_name[64] = {0};
    snprintf(dev_name, sizeof(dev_name), dev, fbnum);
    return fd.open(dev_name, flags);
}

inline OvFD::OvFD() : mFD (INVAL) {
    mPath[0] = 0;
}

inline OvFD::~OvFD() {
    //no op since copy() can be used to share fd, in 3d cases.
}

inline bool OvFD::open(const char* const dev, int flags)
{
    mFD = ::open(dev, flags, 0);
    if (mFD < 0) {
        // FIXME errno, strerror in bionic?
        ALOGE("Cant open device %s err=%d", dev, errno);
        return false;
    }
    setPath(dev);
    return true;
}

inline void OvFD::setPath(const char* const dev)
{
    ::strncpy(mPath, dev, utils::MAX_PATH_LEN);
}

inline bool OvFD::close()
{
    int ret = 0;
    if(valid()) {
        ret = ::close(mFD);
        mFD = INVAL;
    }
    return (ret == 0);
}

inline bool OvFD::valid() const
{
    return (mFD != INVAL);
}

inline int OvFD::getFD() const { return mFD; }

inline void OvFD::copy(int fd) {
    mFD = fd;
}

inline void OvFD::dump() const
{
    ALOGE("== Dump OvFD fd=%d path=%s start/end ==",
            mFD, mPath);
}

//--------------- class OvFD stuff ends ---------------------

} // overlay


#endif // OVERLAY_UTILS_H
