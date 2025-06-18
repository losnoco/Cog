#ifndef __SOUNDEMU_H__
#define __SOUNDEMU_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "../stdtype.h"
#include "EmuStructs.h"

/**
 * @brief Retrieve a list of all available sound cores for a device. Uses built-in sound devices.
 *        [deprecated - use SndEmu_GetDevDecl instead]
 *
 * @param deviceID ID of the sound device (see DEVID_ constants in SoundDevs.h)
 * @return an array of DEV_DEF* that is terminated by a NULL pointer
 */
const DEV_DEF* const* SndEmu_GetDevDefList(DEV_ID deviceID);
/**
 * @brief Return the device declaration of a sound device.
 *        Devices in the user-specified device list take priority.
 *
 * @param deviceID ID of the sound device (see DEVID_ constants in SoundDevs.h)
 * @param userDevList user-supplied sound device list, must be terminated with a NULL pointer, list can be NULL
 * @param opts option flags, see EST_OPT constants
 * @return pointer to device declaration or NULL if the device is not found
 */
const DEV_DECL* SndEmu_GetDevDecl(DEV_ID deviceID, const DEV_DECL** userDevList, UINT8 opts);
/**
 * @brief Initializes emulation for a sound device. Uses built-in sound devices.
 *
 * @param deviceID ID of the sound device to be emulated (see DEVID constants in SoundDevs.h)
 * @param cfg chip-dependent configuration structure, contains various settings
 * @param retDevInf pointer to DEV_INFO structure that gets filled with device information,
 *        caller has to free information about linkable devices
 * @return error code. 0 = success, see EERR constants
 */
UINT8 SndEmu_Start(DEV_ID deviceID, const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf);
/**
 * @brief Initializes emulation for a sound device.
 *        Devices in the user-specified device list take priority with fallback to the default list for missing cores.
 *
 * @param deviceID ID of the sound device to be emulated (see DEVID constants in SoundDevs.h)
 * @param cfg chip-dependent configuration structure, contains various settings
 * @param retDevInf pointer to DEV_INFO structure that gets filled with device information,
 *        caller has to free information about linkable devices
 * @param userDevList user-supplied sound device list, must be terminated with a NULL pointer, list can be NULL
 * @param opts option flags, see EST_OPT constants
 * @return error code. 0 = success, see EERR constants
 */
UINT8 SndEmu_Start2(DEV_ID deviceID, const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf, const DEV_DECL** userDevList, UINT8 opts);
/**
 * @brief Deinitializes the sound core.
 *
 * @param devInf DEV_INFO structure of the device to be stopped
 * @return always returns 0 (success)
 */
UINT8 SndEmu_Stop(DEV_INFO* devInf);
/**
 * @brief Frees memory that holds information about linkable devices.
 *        Should be called sometime after a successful SndEmu_Start() in order to prevent memory leaks.
 *
 * @param devInf DEV_INFO structure of the main device
 */
void SndEmu_FreeDevLinkData(DEV_INFO* devInf);
/**
 * @brief Retrieve a function of a sound core that fullfills certain conditions.
 *
 * @param devInf DEV_INFO structure of the device
 * @param funcType function type (write/read, register/memory, ...), see RWF_ constants in EmuStructs.h
 * @param rwType read/write data type, see DEVRW_ constants in EmuStructs.h
 * @param user user-defined value for distinguishing functions with the same funcType/rwType, 0 = default
 * @param retFuncPtr parameter the function pointer is stored in
 * @return error code. 0 = success, 1 - success, but more possible candidates found, see EERR constants
 */
UINT8 SndEmu_GetDeviceFunc(const DEV_DEF* devInf, UINT8 funcType, UINT8 rwType, UINT16 user, void** retFuncPtr);
/**
 * @brief Retrieve the name of a sound device.
 *        Device configuration parameters may be use to identify exact sound chip models.
 *
 * @param deviceID ID of the sound device to get the name of (see DEVID constants in SoundDevs.h)
 * @param opts bitfield of options
 *             Bit 0 (0x01): enable long names
 * @param cfg chip-dependent configuration structure, allows for correct names of device variations,
 *            ONLY used when long names are enabled
 * @return pointer to name of the device
 */
const char* SndEmu_GetDevName(DEV_ID deviceID, UINT8 opts, const DEV_GEN_CFG* devCfg);


extern const DEV_DECL* sndEmu_Devices[];	// list of built-in sound devices, terminated by a NULL pointer


#define EST_OPT_NO_DEFAULT	0x01	// SndEmu_Start option: don't use default built-in device list
#define EST_OPT_STRICT_OVRD	0x02	// SndEmu_Start option: strict override (no fallback for missing cores)

#define EERR_OK			0x00
#define EERR_MORE_FOUND	0x01	// success, but more items were found
#define EERR_UNK_DEVICE	0xF0	// unknown/invalid device ID
#define EERR_NOT_FOUND	0xF8	// sound core or function not found
#define EERR_INIT_ERR	0xFF	// sound core initialization error (usually malloc error)

#ifdef __cplusplus
}
#endif

#endif	// __SOUNDEMU_H__
