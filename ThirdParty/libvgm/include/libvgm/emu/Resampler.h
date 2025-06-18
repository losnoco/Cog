#ifndef __RESAMPLER_H__
#define __RESAMPLER_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "../stdtype.h"
#include "snddef.h"	// for DEV_SMPL
#include "EmuStructs.h"

typedef struct _waveform_32bit_stereo WAVE_32BS;
typedef struct _resampling_state RESMPL_STATE;

typedef void (*RESAMPLER_FUNC)(RESMPL_STATE* CAA, UINT32 length, WAVE_32BS* retSample);

struct _waveform_32bit_stereo
{
	DEV_SMPL L;
	DEV_SMPL R;
};
// Resampler Modes
#define RSMODE_LINEAR	0x00	// linear interpolation (good quality)
#define RSMODE_NEAREST	0x01	// nearest-neighbour (low quality)
#define RSMODE_LUP_NDWN	0x02	// nearest-neighbour downsampling, interpolation upsampling
struct _resampling_state
{
	UINT32 smpRateSrc;
	UINT32 smpRateDst;
	INT16 volumeL;
	INT16 volumeR;
	UINT8 resampleMode;	// see RSMODE_ constants
	RESAMPLER_FUNC resampler;
	DEVFUNC_UPDATE StreamUpdate;
	void* su_DataPtr;
	UINT32 smpP;		// Current Sample (Playback Rate)
	UINT32 smpLast;		// Sample Number Last
	UINT32 smpNext;		// Sample Number Next
	WAVE_32BS lSmpl;	// Last Sample
	WAVE_32BS nSmpl;	// Next Sample
	UINT32 smplBufSize;
	DEV_SMPL* smplBufs[2];
};

// ---- resampler helper functions (for quick/comfortable initialization) ----
/**
 * @brief Sets up a the resampler to use a certain sound device.
 *
 * @param CAA resampler to be connected to a device
 * @param devInf device to be used by the resampler
 */
void Resmpl_DevConnect(RESMPL_STATE* CAA, const DEV_INFO* devInf);
/**
 * @brief Helper function to quickly set resampler configuration values.
 *
 * @param CAA resampler to be configured
 * @param resampleMode resampling mode, 0xFF = auto
 * @param volume volume gain applied during resampling process, 8.8 fixed point, 0x100 equals 100%
 * @param destSampleRate sample rate of the output stream
 */
void Resmpl_SetVals(RESMPL_STATE* CAA, UINT8 resampleMode, UINT16 volume, UINT32 destSampleRate);

// ---- resampler main functions ----
/**
 * @brief Initializes a resampler. The resampler must be configured and connected to a device.
 *
 * @param CAA resampler to be initialized
 */
void Resmpl_Init(RESMPL_STATE* CAA);
/**
 * @brief Deinitializes a resampler and frees used memory.
 *
 * @param CAA resampler to be deinitialized
 */
void Resmpl_Deinit(RESMPL_STATE* CAA);
/**
 * @brief Sets the sample rate of the sound device. Used for sample rate changes without deinit/init.
 *
 * @param CAA resampler whose input sample rate is changed
 */
void Resmpl_ChangeRate(void* DataPtr, UINT32 newSmplRate);
/**
 * @brief Request and resample input data in order to render samples into the output buffer.
 *
 * @param CAA resampler to be executed
 * @param samples number of output samples to be rendered
 * @param smplBuffer buffer for output data
 */
void Resmpl_Execute(RESMPL_STATE* CAA, UINT32 samples, WAVE_32BS* smplBuffer);

#ifdef __cplusplus
}
#endif

#endif	// __RESAMPLER_H__
