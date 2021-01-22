QT       += core gui network serialport multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

macx:LIBS += -framework AVFoundation
macx: QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.14
macx:QMAKE_INFO_PLIST = Info.plist

TARGET = dudestar
TEMPLATE = app
VERSION_BUILD='$(shell cd $$PWD;git rev-parse --short HEAD)'
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += VERSION_NUMBER=\"\\\"$${VERSION_BUILD}\\\"\"
#DEFINES += USE_FLITE
DEFINES += USE_SWTX
CONFIG += c++11

SOURCES += \
        CRCenc.cpp \
        DMRData.cpp \
        Golay24128.cpp \
        SHA256.cpp \
        YSFConvolution.cpp \
        YSFFICH.cpp \
        ambe.c \
        ambe3600x2400.c \
        ambe3600x2450.c \
        audioengine.cpp \
        cbptc19696.cpp \
        cgolay2087.cpp \
        chamming.cpp \
        codec2/codebooks.cpp \
        codec2/codec2.cpp \
        codec2/kiss_fft.cpp \
        codec2/lpc.cpp \
        codec2/nlp.cpp \
        codec2/pack.cpp \
        codec2/qbase.cpp \
        codec2/quantise.cpp \
        crs129.cpp \
        dcscodec.cpp \
        dmrcodec.cpp \
        dudestar.cpp \
        ecc.c \
		httpmanager.cpp \
        iaxcodec.cpp \
        imbe7200x4400.c \
        imbe_vocoder/aux_sub.cc \
        imbe_vocoder/basicop2.cc \
        imbe_vocoder/ch_decode.cc \
        imbe_vocoder/ch_encode.cc \
        imbe_vocoder/dc_rmv.cc \
        imbe_vocoder/decode.cc \
        imbe_vocoder/dsp_sub.cc \
        imbe_vocoder/encode.cc \
        imbe_vocoder/imbe_vocoder.cc \
        imbe_vocoder/math_sub.cc \
        imbe_vocoder/pe_lpf.cc \
        imbe_vocoder/pitch_est.cc \
        imbe_vocoder/pitch_ref.cc \
        imbe_vocoder/qnt_sub.cc \
        imbe_vocoder/rand_gen.cc \
        imbe_vocoder/sa_decode.cc \
        imbe_vocoder/sa_encode.cc \
        imbe_vocoder/sa_enh.cc \
        imbe_vocoder/tbls.cc \
        imbe_vocoder/uv_synt.cc \
        imbe_vocoder/v_synt.cc \
        imbe_vocoder/v_uv_det.cc \
        levelmeter.cpp \
        m17codec.cpp \
        main.cpp \
        mbedec.cpp \
        mbeenc.cc \
        mbelib.c \
        nxdncodec.cpp \
        p25codec.cpp \
        refcodec.cpp \
        serialambe.cpp \
        xrfcodec.cpp \
        ysfcodec.cpp
macx:OBJECTIVE_SOURCES += micpermission.mm

HEADERS += \
        CRCenc.h \
        DMRData.h \
        DMRDefines.h \
        Golay24128.h \
        SHA256.h \
        YSFConvolution.h \
        YSFFICH.h \
        ambe.h \
        ambe3600x2400_const.h \
        ambe3600x2450_const.h \
        audioengine.h \
        cbptc19696.h \
        cgolay2087.h \
        chamming.h \
        codec2/codec2.h \
        codec2/codec2_internal.h \
        codec2/defines.h \
        codec2/kiss_fft.h \
        codec2/lpc.h \
        codec2/nlp.h \
        codec2/qbase.h \
        codec2/quantise.h \
        crs129.h \
        dcscodec.h \
        dmrcodec.h \
        dudestar.h \
        ecc_const.h \
		httpmanager.h \
        iaxcodec.h \
        iaxdefines.h \
        imbe7200x4400_const.h \
        imbe_vocoder/aux_sub.h \
        imbe_vocoder/basic_op.h \
        imbe_vocoder/ch_decode.h \
        imbe_vocoder/ch_encode.h \
        imbe_vocoder/dc_rmv.h \
        imbe_vocoder/decode.h \
        imbe_vocoder/dsp_sub.h \
        imbe_vocoder/encode.h \
        imbe_vocoder/globals.h \
        imbe_vocoder/imbe.h \
        imbe_vocoder/imbe_vocoder.h \
        imbe_vocoder/math_sub.h \
        imbe_vocoder/pe_lpf.h \
        imbe_vocoder/pitch_est.h \
        imbe_vocoder/pitch_ref.h \
        imbe_vocoder/qnt_sub.h \
        imbe_vocoder/rand_gen.h \
        imbe_vocoder/sa_decode.h \
        imbe_vocoder/sa_encode.h \
        imbe_vocoder/sa_enh.h \
        imbe_vocoder/tbls.h \
        imbe_vocoder/typedef.h \
        imbe_vocoder/typedefs.h \
        imbe_vocoder/uv_synt.h \
        imbe_vocoder/v_synt.h \
        imbe_vocoder/v_uv_det.h \
        levelmeter.h \
        m17codec.h \
        mbedec.h \
        mbeenc.h \
        mbelib_const.h \
        mbelib_parms.h \
        nxdncodec.h \
        p25codec.h \
        refcodec.h \
        serialambe.h \
        vocoder_tables.h \
        xrfcodec.h \
        ysfcodec.h
macx:HEADERS += micpermission.h

FORMS += \
    dudestar.ui

win32:QMAKE_LFLAGS += -static

QMAKE_LFLAGS_WINDOWS += --enable-stdcall-fixup

contains(DEFINES, USE_FLITE){
	LIBS += -lflite_cmu_us_slt -lflite_cmu_us_kal16 -lflite_cmu_us_awb -lflite_cmu_us_rms -lflite_usenglish -lflite_cmulex -lflite -lasound
}
RC_ICONS = images/dudestar.ico
ICON = images/DudeStar.icns

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    dudestar.qrc

DISTFILES +=
