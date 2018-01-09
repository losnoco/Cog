#pragma once

#define CONFIG_DATA_SIZE 4
#define MAX_BLOCK_COUNT 5
#define MAX_BLOCK_CHANNEL_COUNT 2
#define MAX_FRAME_SAMPLES 256
#define MAX_BEX_VALUES 4

#define MAX_QUANT_UNITS 30

typedef struct frame frame;
typedef struct block block;

typedef enum BlockType {
	Mono = 0,
	Stereo = 1,
	LFE = 2
} BlockType;

typedef struct {
	int BlockCount;
	int ChannelCount;
	enum BlockType Types[MAX_BLOCK_COUNT];
} ChannelConfig;

typedef struct {
	unsigned char ConfigData[CONFIG_DATA_SIZE];
	int SampleRateIndex;
	int ChannelConfigIndex;
	int FrameBytes;
	int SuperframeIndex;

	ChannelConfig ChannelConfig;
	int ChannelCount;
	int SampleRate;
	int HighSampleRate;
	int FramesPerSuperframe;
	int FrameSamplesPower;
	int FrameSamples;
	int SuperframeBytes;
	int SuperframeSamples;
} ConfigData;

typedef struct {
	int initialized;
	unsigned short stateA;
	unsigned short stateB;
	unsigned short stateC;
	unsigned short stateD;
} rng_cxt;

typedef struct {
	int bits;
	int size;
	double scale;
	double _imdctPrevious[MAX_FRAME_SAMPLES];
	double* window;
	double* sinTable;
	double* cosTable;
} mdct;

typedef struct {
	frame* Frame;
	block* Block;
	ConfigData* config;
	int ChannelIndex;

	mdct mdct;

	double Pcm[MAX_FRAME_SAMPLES];
	double Spectra[MAX_FRAME_SAMPLES];

	int CodedQuantUnits;
	int ScaleFactorCodingMode;

	int ScaleFactors[31];
	int ScaleFactorsPrev[31];

	int Precisions[MAX_QUANT_UNITS];
	int PrecisionsFine[MAX_QUANT_UNITS];
	int PrecisionMask[MAX_QUANT_UNITS];

	int CodebookSet[MAX_QUANT_UNITS];

	int QuantizedSpectra[MAX_FRAME_SAMPLES];
	int QuantizedSpectraFine[MAX_FRAME_SAMPLES];

	int BexMode;
	int BexValueCount;
	int BexValues[MAX_BEX_VALUES];

	rng_cxt rng;
} channel;

struct block {
	frame* Frame;
	ConfigData* config;
	enum BlockType BlockType;
	int BlockIndex;
	channel Channels[MAX_BLOCK_CHANNEL_COUNT];
	int ChannelCount;
	int FirstInSuperframe;
	int ReuseBandParams;

	int BandCount;
	int StereoBand;
	int ExtensionBand;
	int QuantizationUnitCount;
	int StereoQuantizationUnit;
	int ExtensionUnit;
	int QuantizationUnitsPrev;

	int Gradient[31];
	int GradientMode;
	int GradientStartUnit;
	int GradientStartValue;
	int GradientEndUnit;
	int GradientEndValue;
	int GradientBoundary;

	int PrimaryChannelIndex;
	int HasJointStereoSigns;
	int JointStereoSigns[MAX_QUANT_UNITS];

	int BandExtensionEnabled;
	int HasExtensionData;
	int BexDataLength;
	int BexMode;
};

struct frame {
	ConfigData* config;
	int FrameIndex;
	block Blocks[MAX_BLOCK_COUNT];
	int frameNum;
};

typedef struct {
	int initialized;
	int wlength;
	ConfigData config;
	frame frame;
} atrac9_handle;

typedef struct {
	int group_b_unit;
	int group_c_unit;
	int band_count;
} bex_group;

typedef struct {
	int channels;
	int channelConfigIndex;
	int samplingRate;
	int superframeSize;
	int framesInSuperframe;
	int frameSamples;
	int wlength;
	unsigned char configData[CONFIG_DATA_SIZE];
} CodecInfo;
