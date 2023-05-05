/*
 * PluginManager.h
 * ---------------
 * Purpose: Plugin management
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

OPENMPT_NAMESPACE_BEGIN

constexpr int32 PLUGMAGIC(char a, char b, char c, char d) noexcept
{
	return static_cast<int32>((static_cast<uint32>(a) << 24) | (static_cast<uint32>(b) << 16) | (static_cast<uint32>(c) << 8) | (static_cast<uint32>(d) << 0));
}

//#define kBuzzMagic	PLUGMAGIC('B', 'u', 'z', 'z')
inline constexpr int32 kDmoMagic = PLUGMAGIC('D', 'X', 'M', 'O');

class CSoundFile;
class IMixPlugin;
struct SNDMIXPLUGIN;
enum PluginArch : int;

struct VSTPluginLib
{
public:
	enum PluginCategory : uint8
	{
		// Same plugin categories as defined in VST SDK
		catUnknown = 0,
		catEffect,          // Simple Effect
		catSynth,           // VST Instrument (Synths, samplers,...)
		catAnalysis,        // Scope, Tuner, ...
		catMastering,       // Dynamics, ...
		catSpacializer,     // Panners, ...
		catRoomFx,          // Delays and Reverbs
		catSurroundFx,      // Dedicated surround processor
		catRestoration,     // Denoiser, ...
		catOfflineProcess,  // Offline Process
		catShell,           // Plug-in is container of other plug-ins
		catGenerator,       // Tone Generator, ...
		// Custom categories
		catDMO,     // DirectX media object plugin
		catHidden,  // For internal plugins that should not be visible to the user (e.g. because they only exist for legacy reasons)

		numCategories
	};

public:
	using CreateProc = IMixPlugin *(*)(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN &mixStruct);

	IMixPlugin *pPluginsList = nullptr; // Pointer to first plugin instance (this instance carries pointers to other instances)
	void InsertPluginInstanceIntoList(IMixPlugin &pluginInstance);
	void RemovePluginInstanceFromList(IMixPlugin &pluginInstance);

	CreateProc Create;                  // Factory to call for this plugin
	mpt::PathString libraryName;        // Display name
	mpt::PathString dllPath;            // Full path name
#ifdef MODPLUG_TRACKER
	mpt::ustring tags;                  // User tags
	CString vendor;
#endif // MODPLUG_TRACKER
	int32 pluginId1 = 0;                // Plugin type (kEffectMagic, kDmoMagic, ...)
	int32 pluginId2 = 0;                // Plugin unique ID
	PluginCategory category = catUnknown;
	const bool isBuiltIn : 1;
	bool isInstrument : 1;
	bool useBridge : 1, shareBridgeInstance : 1, modernBridge : 1;
protected:
	mutable uint8 dllArch = 0;

public:
	VSTPluginLib(CreateProc factoryProc, bool isBuiltIn, const mpt::PathString &dllPath, const mpt::PathString &libraryName
#ifdef MODPLUG_TRACKER
		, const mpt::ustring &tags = mpt::ustring(), const CString &vendor = CString()
#endif // MODPLUG_TRACKER
		)
		: Create(factoryProc)
		, libraryName(libraryName), dllPath(dllPath)
#ifdef MODPLUG_TRACKER
		, tags(tags)
		, vendor(vendor)
#endif // MODPLUG_TRACKER
		, category(catUnknown)
		, isBuiltIn(isBuiltIn), isInstrument(false)
		, useBridge(false), shareBridgeInstance(true), modernBridge(true)
	{
	}

#ifdef MPT_WITH_VST

	// Get native phost process arch encoded as plugin arch
	static uint8 GetNativePluginArch();
	static mpt::ustring GetPluginArchName(uint8 arch);
	static mpt::ustring GetPluginArchNameUser(uint8 arch);

	// Check whether a plugin can be hosted inside OpenMPT or requires bridging
	uint8 GetDllArch(bool fromCache = true) const;
	mpt::ustring GetDllArchName(bool fromCache = true) const;
	mpt::ustring GetDllArchNameUser(bool fromCache = true) const;
	bool IsNative(bool fromCache = true) const;
	// Check if a plugin is native, and if it is currently unknown, assume that it is native. Use this function only for performance reasons
	// (e.g. if tons of unscanned plugins would slow down generation of the plugin selection dialog)
	bool IsNativeFromCache() const;

#endif // MPT_WITH_VST

	void WriteToCache() const;

	uint32 EncodeCacheFlags() const
	{
		// Format: 00000000.0000000M.AAAAAASB.CCCCCCCI
		return (isInstrument ? 1 : 0)
			| (category << 1)
			| (useBridge ? 0x100 : 0)
			| (shareBridgeInstance ? 0x200 : 0)
			| ((dllArch / 8) << 10)
			| (modernBridge ? 0x10000 : 0)
			;
	}

	void DecodeCacheFlags(uint32 flags)
	{
		category = static_cast<PluginCategory>((flags & 0xFF) >> 1);
		if(category >= numCategories)
		{
			category = catUnknown;
		}
		if(flags & 1)
		{
			isInstrument = true;
			category = catSynth;
		}
		useBridge = (flags & 0x100) != 0;
		shareBridgeInstance = (flags & 0x200) != 0;
		dllArch = ((flags >> 10) & 0x3F) * 8;
		modernBridge = (flags & 0x10000) != 0;
	}
};


class CVstPluginManager
{
#ifndef NO_PLUGINS
protected:
#if defined(MPT_WITH_DMO)
	bool MustUnInitilizeCOM = false;
#endif
	std::vector<VSTPluginLib *> pluginList;

public:
	CVstPluginManager();
	~CVstPluginManager();

	using iterator = std::vector<VSTPluginLib *>::iterator;
	using const_iterator = std::vector<VSTPluginLib *>::const_iterator;

	iterator begin() { return pluginList.begin(); }
	const_iterator begin() const { return pluginList.begin(); }
	iterator end() { return pluginList.end(); }
	const_iterator end() const { return pluginList.end(); }
	void reserve(size_t num) { pluginList.reserve(num); }
	size_t size() const { return pluginList.size(); }

	bool IsValidPlugin(const VSTPluginLib *pLib) const;
	VSTPluginLib *AddPlugin(const mpt::PathString &dllPath, bool maskCrashes, const mpt::ustring &tags = mpt::ustring(), bool fromCache = true, bool *fileFound = nullptr);
	bool RemovePlugin(VSTPluginLib *);
	bool CreateMixPlugin(SNDMIXPLUGIN &, CSoundFile &);
	void OnIdle();
	static void ReportPlugException(const mpt::ustring &msg);

protected:
	void EnumerateDirectXDMOs();

#else // NO_PLUGINS
public:
	const VSTPluginLib **begin() const { return nullptr; }
	const VSTPluginLib **end() const { return nullptr; }
	void reserve(size_t) { }
	size_t size() const { return 0; }

	void OnIdle() {}
#endif // NO_PLUGINS
};


OPENMPT_NAMESPACE_END
