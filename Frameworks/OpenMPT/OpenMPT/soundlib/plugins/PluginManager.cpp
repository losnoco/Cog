/*
 * PluginManager.cpp
 * -----------------
 * Purpose: Implementation of the plugin manager, which keeps a list of known plugins and instantiates them.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#ifndef NO_PLUGINS

#include "../../common/version.h"
#include "PluginManager.h"
#include "PlugInterface.h"

#if defined(MPT_WITH_DMO)
#include "mpt/uuid/guid.hpp"
#endif // MPT_WITH_DMO
#include "mpt/uuid/uuid.hpp"

// Built-in plugins
#include "DigiBoosterEcho.h"
#include "LFOPlugin.h"
#include "SymMODEcho.h"
#include "dmo/DMOPlugin.h"
#include "dmo/Chorus.h"
#include "dmo/Compressor.h"
#include "dmo/Distortion.h"
#include "dmo/Echo.h"
#include "dmo/Flanger.h"
#include "dmo/Gargle.h"
#include "dmo/I3DL2Reverb.h"
#include "dmo/ParamEq.h"
#include "dmo/WavesReverb.h"
#ifdef MODPLUG_TRACKER
#include "../../mptrack/plugins/MidiInOut.h"
#endif // MODPLUG_TRACKER

#include "../../common/mptStringBuffer.h"
#include "../Sndfile.h"
#include "../Loaders.h"

#ifdef MPT_WITH_VST
#include "../../mptrack/Vstplug.h"
#include "../../pluginBridge/BridgeWrapper.h"
#ifdef MODPLUG_TRACKER
#include "mpt/fs/fs.hpp"
#endif // MODPLUG_TRACKER
#endif // MPT_WITH_VST

#if defined(MPT_WITH_DMO)
#include <winreg.h>
#include <strmif.h>
#include <tchar.h>
#endif // MPT_WITH_DMO

#ifdef MODPLUG_TRACKER
#include "../../mptrack/Mptrack.h"
#include "../../mptrack/TrackerSettings.h"
#include "../../mptrack/AbstractVstEditor.h"
#include "../../soundlib/AudioCriticalSection.h"
#include "../mptrack/ExceptionHandler.h"
#include "mpt/crc/crc.hpp"
#endif // MODPLUG_TRACKER


OPENMPT_NAMESPACE_BEGIN


using namespace mpt::uuid_literals;


#ifdef MPT_ALL_LOGGING
#define VST_LOG
#define DMO_LOG
#endif

#ifdef MODPLUG_TRACKER
static constexpr const mpt::uchar *cacheSection = UL_("PluginCache");
#endif // MODPLUG_TRACKER


#ifdef MPT_WITH_VST


uint8 VSTPluginLib::GetNativePluginArch()
{
	uint8 result = 0;
	switch(mpt::OS::Windows::GetProcessArchitecture())
	{
	case mpt::OS::Windows::Architecture::x86:
		result = PluginArch_x86;
		break;
	case mpt::OS::Windows::Architecture::amd64:
		result = PluginArch_amd64;
		break;
	case mpt::OS::Windows::Architecture::arm:
		result = PluginArch_arm;
		break;
	case mpt::OS::Windows::Architecture::arm64:
		result = PluginArch_arm64;
		break;
	default:
		result = 0;
		break;
	}
	return result;
}


mpt::ustring VSTPluginLib::GetPluginArchName(uint8 arch)
{
	mpt::ustring result;
	switch(arch)
	{
	case PluginArch_x86:
		result = U_("x86");
		break;
	case PluginArch_amd64:
		result = U_("amd64");
		break;
	case PluginArch_arm:
		result = U_("arm");
		break;
	case PluginArch_arm64:
		result = U_("arm64");
		break;
	default:
		result = U_("");
		break;
	}
	return result;
}


mpt::ustring VSTPluginLib::GetPluginArchNameUser(uint8 arch)
{
	mpt::ustring result;
	#if defined(MPT_WITH_WINDOWS10)
		switch(arch)
		{
		case PluginArch_x86:
			result = U_("x86 (32bit)");
			break;
		case PluginArch_amd64:
			result = U_("amd64 (64bit)");
			break;
		case PluginArch_arm:
			result = U_("arm (32bit)");
			break;
		case PluginArch_arm64:
			result = U_("arm64 (64bit)");
			break;
		default:
			result = U_("");
			break;
		}
	#else // !MPT_WITH_WINDOWS10
		switch(arch)
		{
		case PluginArch_x86:
			result = U_("32-Bit");
			break;
		case PluginArch_amd64:
			result = U_("64-Bit");
			break;
		case PluginArch_arm:
			result = U_("32-Bit");
			break;
		case PluginArch_arm64:
			result = U_("64-Bit");
			break;
		default:
			result = U_("");
			break;
		}
	#endif // MPT_WITH_WINDOWS10
	return result;
}


uint8 VSTPluginLib::GetDllArch(bool fromCache) const
{
	// Built-in plugins are always native.
	if(dllPath.empty())
		return GetNativePluginArch();
#ifdef MPT_WITH_VST
	if(!dllArch || !fromCache)
	{
		dllArch = static_cast<uint8>(BridgeWrapper::GetPluginBinaryType(dllPath));
	}
#else // !MPT_WITH_VST
	MPT_UNREFERENCED_PARAMETER(fromCache);
#endif // MPT_WITH_VST
	return dllArch;
}


mpt::ustring VSTPluginLib::GetDllArchName(bool fromCache) const
{
	return GetPluginArchName(GetDllArch(fromCache));
}


mpt::ustring VSTPluginLib::GetDllArchNameUser(bool fromCache) const
{
	return GetPluginArchNameUser(GetDllArch(fromCache));
}


bool VSTPluginLib::IsNative(bool fromCache) const
{
	return GetDllArch(fromCache) == GetNativePluginArch();
}


bool VSTPluginLib::IsNativeFromCache() const
{
	return dllArch == GetNativePluginArch() || dllArch == 0;
}


#endif // MPT_WITH_VST


// PluginCache format:
// FullDllPath = <ID1><ID2><CRC32> (hex-encoded)
// <ID1><ID2><CRC32>.Flags = Plugin Flags (see VSTPluginLib::DecodeCacheFlags).
// <ID1><ID2><CRC32>.Vendor = Plugin Vendor String.

#ifdef MODPLUG_TRACKER
void VSTPluginLib::WriteToCache() const
{
	SettingsContainer &cacheFile = theApp.GetPluginCache();

	const std::string crcName = dllPath.ToUTF8();
	const mpt::crc32 crc(crcName);
	const mpt::ustring IDs = mpt::ufmt::HEX0<8>(pluginId1) + mpt::ufmt::HEX0<8>(pluginId2) + mpt::ufmt::HEX0<8>(crc.result());

	mpt::PathString writePath = dllPath;
	if(theApp.IsPortableMode())
	{
		writePath = theApp.PathAbsoluteToInstallRelative(writePath);
	}

	cacheFile.Write<mpt::ustring>(cacheSection, writePath.ToUnicode(), IDs);
	cacheFile.Write<CString>(cacheSection, IDs + U_(".Vendor"), vendor);
	cacheFile.Write<int32>(cacheSection, IDs + U_(".Flags"), EncodeCacheFlags());
}
#endif // MODPLUG_TRACKER


void VSTPluginLib::InsertPluginInstanceIntoList(IMixPlugin &pluginInstance)
{
	pluginInstance.m_pNext = pPluginsList;
	if(pPluginsList)
	{
		pPluginsList->m_pPrev = &pluginInstance;
	}
	pPluginsList = &pluginInstance;
}


void VSTPluginLib::RemovePluginInstanceFromList(IMixPlugin &pluginInstance)
{
	if(pPluginsList == &pluginInstance)
	{
		pPluginsList = pluginInstance.m_pNext;
	}
	if(pluginInstance.m_pNext)
	{
		pluginInstance.m_pNext->m_pPrev = pluginInstance.m_pPrev;
	}
	if(pluginInstance.m_pPrev)
	{
		pluginInstance.m_pPrev->m_pNext = pluginInstance.m_pNext;
	}
	pluginInstance.m_pPrev = nullptr;
	pluginInstance.m_pNext = nullptr;
}


bool CreateMixPluginProc(SNDMIXPLUGIN &mixPlugin, CSoundFile &sndFile)
{
#ifdef MODPLUG_TRACKER
	CVstPluginManager *that = theApp.GetPluginManager();
	if(that)
	{
		return that->CreateMixPlugin(mixPlugin, sndFile);
	}
	return false;
#else
	if(!sndFile.m_PluginManager)
	{
		sndFile.m_PluginManager = std::make_unique<CVstPluginManager>();
	}
	return sndFile.m_PluginManager->CreateMixPlugin(mixPlugin, sndFile);
#endif // MODPLUG_TRACKER
}


CVstPluginManager::CVstPluginManager()
{
#if defined(MPT_WITH_DMO)
	HRESULT COMinit = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if(COMinit == S_OK || COMinit == S_FALSE)
	{
		MustUnInitilizeCOM = true;
	}
#endif

	// Hard-coded "plugins"
	static constexpr struct
	{
		VSTPluginLib::CreateProc createProc;
		const char *filename, *name;
		uint32 pluginId1, pluginId2;
		VSTPluginLib::PluginCategory category;
		bool isInstrument, isOurs;
	} BuiltInPlugins[] =
	{
		// DirectX Media Objects Emulation
		{ DMO::Chorus::Create,      "{EFE6629C-81F7-4281-BD91-C9D604A95AF6}", "Chorus",      kDmoMagic, 0xEFE6629C, VSTPluginLib::catDMO, false, false },
		{ DMO::Compressor::Create,  "{EF011F79-4000-406D-87AF-BFFB3FC39D57}", "Compressor",  kDmoMagic, 0xEF011F79, VSTPluginLib::catDMO, false, false },
		{ DMO::Distortion::Create,  "{EF114C90-CD1D-484E-96E5-09CFAF912A21}", "Distortion",  kDmoMagic, 0xEF114C90, VSTPluginLib::catDMO, false, false },
		{ DMO::Echo::Create,        "{EF3E932C-D40B-4F51-8CCF-3F98F1B29D5D}", "Echo",        kDmoMagic, 0xEF3E932C, VSTPluginLib::catDMO, false, false },
		{ DMO::Flanger::Create,     "{EFCA3D92-DFD8-4672-A603-7420894BAD98}", "Flanger",     kDmoMagic, 0xEFCA3D92, VSTPluginLib::catDMO, false, false },
		{ DMO::Gargle::Create,      "{DAFD8210-5711-4B91-9FE3-F75B7AE279BF}", "Gargle",      kDmoMagic, 0xDAFD8210, VSTPluginLib::catDMO, false, false },
		{ DMO::I3DL2Reverb::Create, "{EF985E71-D5C7-42D4-BA4D-2D073E2E96F4}", "I3DL2Reverb", kDmoMagic, 0xEF985E71, VSTPluginLib::catDMO, false, false },
		{ DMO::ParamEq::Create,     "{120CED89-3BF4-4173-A132-3CB406CF3231}", "ParamEq",     kDmoMagic, 0x120CED89, VSTPluginLib::catDMO, false, false },
		{ DMO::WavesReverb::Create, "{87FC0268-9A55-4360-95AA-004A1D9DE26C}", "WavesReverb", kDmoMagic, 0x87FC0268, VSTPluginLib::catDMO, false, false },
		// First (inaccurate) Flanger implementation (will be chosen based on library name, shares ID1 and ID2 with regular Flanger)
		{ DMO::Flanger::CreateLegacy, "{EFCA3D92-DFD8-4672-A603-7420894BAD98}", "Flanger (Legacy)", kDmoMagic, 0xEFCA3D92, VSTPluginLib::catHidden, false, false },
		// DigiBooster Pro Echo DSP
		{ DigiBoosterEcho::Create, "", "DigiBooster Pro Echo", MagicLE("DBM0"), MagicLE("Echo"), VSTPluginLib::catRoomFx, false, true },
		// LFO
		{ LFOPlugin::Create, "", "LFO", MagicLE("OMPT"), MagicLE("LFO "), VSTPluginLib::catGenerator, false, true },
		// SymMOD Echo
		{ SymMODEcho::Create, "", "SymMOD Echo", MagicLE("SymM"), MagicLE("Echo"), VSTPluginLib::catRoomFx, false, true },
#ifdef MODPLUG_TRACKER
		{ MidiInOut::Create, "", "MIDI Input Output", PLUGMAGIC('V','s','t','P'), PLUGMAGIC('M','M','I','D'), VSTPluginLib::catSynth, true, true },
#endif // MODPLUG_TRACKER
	};

	pluginList.reserve(std::size(BuiltInPlugins));
	for(const auto &plugin : BuiltInPlugins)
	{
		VSTPluginLib *plug = new (std::nothrow) VSTPluginLib(plugin.createProc, true, mpt::PathString::FromUTF8(plugin.filename), mpt::PathString::FromUTF8(plugin.name));
		if(plug != nullptr)
		{
			pluginList.push_back(plug);
			plug->pluginId1 = plugin.pluginId1;
			plug->pluginId2 = plugin.pluginId2;
			plug->category = plugin.category;
			plug->isInstrument = plugin.isInstrument;
#ifdef MODPLUG_TRACKER
			if(plugin.isOurs)
				plug->vendor = _T("OpenMPT Project");
#endif // MODPLUG_TRACKER
		}
	}

#ifdef MODPLUG_TRACKER
	// For security reasons, we do not load untrusted DMO plugins in libopenmpt.
	EnumerateDirectXDMOs();
#endif
}


CVstPluginManager::~CVstPluginManager()
{
	for(auto &plug : pluginList)
	{
		while(plug->pPluginsList != nullptr)
		{
			IMixPlugin *pluginInstance = plug->pPluginsList;
			plug->RemovePluginInstanceFromList(*pluginInstance);
			pluginInstance->Release();
		}
		delete plug;
	}
#if defined(MPT_WITH_DMO)
	if(MustUnInitilizeCOM)
	{
		CoUninitialize();
		MustUnInitilizeCOM = false;
	}
#endif
}


bool CVstPluginManager::IsValidPlugin(const VSTPluginLib *pLib) const
{
	return mpt::contains(pluginList, pLib);
}


void CVstPluginManager::EnumerateDirectXDMOs()
{
#if defined(MPT_WITH_DMO)
	static constexpr mpt::UUID knownDMOs[] =
	{
		"745057C7-F353-4F2D-A7EE-58434477730E"_uuid, // AEC (Acoustic echo cancellation, not usable)
		"EFE6629C-81F7-4281-BD91-C9D604A95AF6"_uuid, // Chorus
		"EF011F79-4000-406D-87AF-BFFB3FC39D57"_uuid, // Compressor
		"EF114C90-CD1D-484E-96E5-09CFAF912A21"_uuid, // Distortion
		"EF3E932C-D40B-4F51-8CCF-3F98F1B29D5D"_uuid, // Echo
		"EFCA3D92-DFD8-4672-A603-7420894BAD98"_uuid, // Flanger
		"DAFD8210-5711-4B91-9FE3-F75B7AE279BF"_uuid, // Gargle
		"EF985E71-D5C7-42D4-BA4D-2D073E2E96F4"_uuid, // I3DL2Reverb
		"120CED89-3BF4-4173-A132-3CB406CF3231"_uuid, // ParamEq
		"87FC0268-9A55-4360-95AA-004A1D9DE26C"_uuid, // WavesReverb
		"F447B69E-1884-4A7E-8055-346F74D6EDB3"_uuid, // Resampler DMO (not usable)
	};

	HKEY hkEnum;
	TCHAR keyname[128];

	LONG cr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("software\\classes\\DirectShow\\MediaObjects\\Categories\\f3602b3f-0592-48df-a4cd-674721e7ebeb"), 0, KEY_READ, &hkEnum);
	DWORD index = 0;
	while (cr == ERROR_SUCCESS)
	{
		if ((cr = RegEnumKey(hkEnum, index, keyname, mpt::saturate_cast<DWORD>(std::size(keyname)))) == ERROR_SUCCESS)
		{
			CLSID clsid;
			mpt::winstring formattedKey = mpt::winstring(_T("{")) + mpt::winstring(keyname) + mpt::winstring(_T("}"));
			if(mpt::VerifyStringToCLSID(formattedKey, clsid))
			{
				if(!mpt::contains(knownDMOs, clsid))
				{
					HKEY hksub;
					formattedKey = mpt::winstring(_T("software\\classes\\DirectShow\\MediaObjects\\")) + mpt::winstring(keyname);
					if (RegOpenKey(HKEY_LOCAL_MACHINE, formattedKey.c_str(), &hksub) == ERROR_SUCCESS)
					{
						TCHAR name[64];
						DWORD datatype = REG_SZ;
						DWORD datasize = sizeof(name);

						if(ERROR_SUCCESS == RegQueryValueEx(hksub, nullptr, 0, &datatype, (LPBYTE)name, &datasize))
						{
							VSTPluginLib *plug = new (std::nothrow) VSTPluginLib(DMOPlugin::Create, true, mpt::PathString::FromNative(mpt::GUIDToString(clsid)), mpt::PathString::FromNative(ParseMaybeNullTerminatedStringFromBufferWithSizeInBytes<mpt::winstring>(name, datasize)));
							if(plug != nullptr)
							{
								try
								{
									pluginList.push_back(plug);
									plug->pluginId1 = kDmoMagic;
									plug->pluginId2 = clsid.Data1;
									plug->category = VSTPluginLib::catDMO;
								} catch(mpt::out_of_memory e)
								{
									mpt::delete_out_of_memory(e);
									delete plug;
								}
#ifdef DMO_LOG
								MPT_LOG_GLOBAL(LogDebug, "DMO", MPT_UFORMAT("Found \"{}\" clsid={}\n")(plug->libraryName, plug->dllPath));
#endif
							}
						}
						RegCloseKey(hksub);
					}
				}
			}
		}
		index++;
	}
	if (hkEnum) RegCloseKey(hkEnum);
#endif // MPT_WITH_DMO
}


// Extract instrument and category information from plugin.
#ifdef MPT_WITH_VST
static void GetPluginInformation(bool maskCrashes, Vst::AEffect *effect, VSTPluginLib &library)
{
	unsigned long exception = 0;
	library.category = static_cast<VSTPluginLib::PluginCategory>(CVstPlugin::DispatchSEH(maskCrashes, effect, Vst::effGetPlugCategory, 0, 0, nullptr, 0, exception));
	library.isInstrument = ((effect->flags & Vst::effFlagsIsSynth) || !effect->numInputs);

	if(library.isInstrument)
	{
		library.category = VSTPluginLib::catSynth;
	} else if(library.category >= VSTPluginLib::numCategories)
	{
		library.category = VSTPluginLib::catUnknown;
	}

#ifdef MODPLUG_TRACKER
	std::vector<char> s(256, 0);
	CVstPlugin::DispatchSEH(maskCrashes, effect, Vst::effGetVendorString, 0, 0, s.data(), 0, exception);
	library.vendor = mpt::ToCString(mpt::Charset::Locale, s.data());
#endif // MODPLUG_TRACKER
}
#endif // MPT_WITH_VST


#ifdef MPT_WITH_VST
static bool TryLoadPlugin(bool maskCrashes, VSTPluginLib *plug, HINSTANCE hLib, unsigned long &exception)
{
	Vst::AEffect *pEffect = CVstPlugin::LoadPlugin(maskCrashes, *plug, hLib, CVstPlugin::BridgeMode::DetectRequiredBridgeMode);
	if(!pEffect || pEffect->magic != Vst::kEffectMagic || !pEffect->dispatcher)
	{
		return false;
	}

	CVstPlugin::DispatchSEH(maskCrashes, pEffect, Vst::effOpen, 0, 0, 0, 0, exception);

	plug->pluginId1 = pEffect->magic;
	plug->pluginId2 = pEffect->uniqueID;

	GetPluginInformation(maskCrashes, pEffect, *plug);

#ifdef VST_LOG
	intptr_t nver = CVstPlugin::DispatchSEH(maskCrashes, pEffect, Vst::effGetVstVersion, 0,0, nullptr, 0, exception);
	if (!nver) nver = pEffect->version;
	MPT_LOG_GLOBAL(LogDebug, "VST", MPT_UFORMAT("{}: v{}.0, {} in, {} out, {} programs, {} params, flags=0x{} realQ={} offQ={}")(
		plug->libraryName, nver,
		pEffect->numInputs, pEffect->numOutputs,
		mpt::ufmt::dec0<2>(pEffect->numPrograms), mpt::ufmt::dec0<2>(pEffect->numParams),
		mpt::ufmt::HEX0<4>(static_cast<int32>(pEffect->flags)), pEffect->realQualities, pEffect->offQualities));
#endif // VST_LOG

	CVstPlugin::DispatchSEH(maskCrashes, pEffect, Vst::effClose, 0, 0, 0, 0, exception);

	return true;
}
#endif // !NO_NVST


#ifdef MODPLUG_TRACKER
// Add a plugin to the list of known plugins.
VSTPluginLib *CVstPluginManager::AddPlugin(const mpt::PathString &dllPath, bool maskCrashes, const mpt::ustring &tags, bool fromCache, bool *fileFound)
{
	const mpt::PathString fileName = dllPath.GetFilenameBase();

	// Check if this is already a known plugin.
	for(const auto &dupePlug : pluginList)
	{
		if(!mpt::PathCompareNoCase(dllPath, dupePlug->dllPath)) return dupePlug;
	}

	if(fileFound != nullptr)
	{
		*fileFound = mpt::native_fs{}.is_file(dllPath);
	}

	// Look if the plugin info is stored in the PluginCache
	if(fromCache)
	{
		SettingsContainer & cacheFile = theApp.GetPluginCache();
		// First try finding the full path
		mpt::ustring IDs = cacheFile.Read<mpt::ustring>(cacheSection, dllPath.ToUnicode(), U_(""));
		if(IDs.length() < 16)
		{
			// If that didn't work out, find relative path
			mpt::PathString relPath = theApp.PathAbsoluteToInstallRelative(dllPath);
			IDs = cacheFile.Read<mpt::ustring>(cacheSection, relPath.ToUnicode(), U_(""));
		}

		if(IDs.length() >= 16)
		{
			VSTPluginLib *plug = new (std::nothrow) VSTPluginLib(nullptr, false, dllPath, fileName, tags);
			if(plug == nullptr)
			{
				return nullptr;
			}
			pluginList.push_back(plug);

			// Extract plugin IDs
			for (int i = 0; i < 16; i++)
			{
				int32 n = IDs[i] - '0';
				if (n > 9) n = IDs[i] + 10 - 'A';
				n &= 0x0f;
				if (i < 8)
				{
					plug->pluginId1 = (plug->pluginId1 << 4) | n;
				} else
				{
					plug->pluginId2 = (plug->pluginId2 << 4) | n;
				}
			}

			const mpt::ustring flagKey = IDs + U_(".Flags");
			plug->DecodeCacheFlags(cacheFile.Read<int32>(cacheSection, flagKey, 0));
			plug->vendor = cacheFile.Read<CString>(cacheSection, IDs + U_(".Vendor"), CString());

#ifdef VST_LOG
			MPT_LOG_GLOBAL(LogDebug, "VST", MPT_UFORMAT("Plugin \"{}\" found in PluginCache")(plug->libraryName));
#endif // VST_LOG
			return plug;
		} else
		{
#ifdef VST_LOG
			MPT_LOG_GLOBAL(LogDebug, "VST", MPT_UFORMAT("Plugin mismatch in PluginCache: \"{}\" [{}]")(dllPath, IDs));
#endif // VST_LOG
		}
	}

	// If this key contains a file name on program launch, a plugin previously crashed OpenMPT.
	theApp.GetSettings().Write<mpt::PathString>(U_("VST Plugins"), U_("FailedPlugin"), dllPath, SettingWriteThrough);

	bool validPlug = false;

	VSTPluginLib *plug = new (std::nothrow) VSTPluginLib(nullptr, false, dllPath, fileName, tags);
	if(plug == nullptr)
	{
		return nullptr;
	}

#ifdef MPT_WITH_VST
	unsigned long exception = 0;
	// Always scan plugins in a separate process
	HINSTANCE hLib = NULL;
	{
#ifdef MODPLUG_TRACKER
		ExceptionHandler::Context ectx{ MPT_UFORMAT("VST Plugin: {}")(plug->dllPath.ToUnicode()) };
		ExceptionHandler::ContextSetter ectxguard{&ectx};
#endif // MODPLUG_TRACKER

		validPlug = TryLoadPlugin(maskCrashes, plug, hLib, exception);
	}
	if(hLib)
	{
		FreeLibrary(hLib);
	}
	if(exception != 0)
	{
		CVstPluginManager::ReportPlugException(MPT_UFORMAT("Exception {} while trying to load plugin \"{}\"!\n")(mpt::ufmt::HEX0<8>(exception), plug->libraryName));
	}

#endif // MPT_WITH_VST

	// Now it should be safe to assume that this plugin loaded properly. :)
	theApp.GetSettings().Remove(U_("VST Plugins"), U_("FailedPlugin"));

	// If OK, write the information in PluginCache
	if(validPlug)
	{
		pluginList.push_back(plug);
		plug->WriteToCache();
	} else
	{
		delete plug;
	}

	return (validPlug ? plug : nullptr);
}


// Remove a plugin from the list of known plugins and release any remaining instances of it.
bool CVstPluginManager::RemovePlugin(VSTPluginLib *pFactory)
{
	for(const_iterator p = begin(); p != end(); p++)
	{
		VSTPluginLib *plug = *p;
		if(plug == pFactory)
		{
			// Kill all instances of this plugin
			CriticalSection cs;

			while(plug->pPluginsList != nullptr)
			{
				IMixPlugin *pluginInstance = plug->pPluginsList;
				plug->RemovePluginInstanceFromList(*pluginInstance);
				pluginInstance->Release();
			}
			pluginList.erase(p);
			delete plug;
			return true;
		}
	}
	return false;
}
#endif // MODPLUG_TRACKER


// Create an instance of a plugin.
bool CVstPluginManager::CreateMixPlugin(SNDMIXPLUGIN &mixPlugin, CSoundFile &sndFile)
{
	VSTPluginLib *pFound = nullptr;

	// Find plugin in library
	enum PlugMatchQuality
	{
		kNoMatch,
		kMatchName,
		kMatchId,
		kMatchNameAndId,
	};

	PlugMatchQuality match = kNoMatch;	// "Match quality" of found plugin. Higher value = better match.
#if MPT_OS_WINDOWS && !MPT_OS_WINDOWS_WINRT
	const mpt::PathString libraryName = mpt::PathString::FromUnicode(mixPlugin.GetLibraryName());
#else
	const std::string libraryName = mpt::ToCharset(mpt::Charset::UTF8, mixPlugin.GetLibraryName());
#endif
	for(const auto &plug : pluginList)
	{
		const bool matchID = (plug->pluginId1 == mixPlugin.Info.dwPluginId1)
			&& (plug->pluginId2 == mixPlugin.Info.dwPluginId2);
#if MPT_OS_WINDOWS && !MPT_OS_WINDOWS_WINRT
		const bool matchName = !mpt::PathCompareNoCase(plug->libraryName, libraryName);
#else
		const bool matchName = !mpt::CompareNoCaseAscii(plug->libraryName.ToUTF8(), libraryName);
#endif

		if(matchID && matchName)
		{
			pFound = plug;
#ifdef MPT_WITH_VST
			if(plug->IsNative(false))
			{
				break;
			}
#endif // MPT_WITH_VST
			// If the plugin isn't native, first check if a native version can be found.
			match = kMatchNameAndId;
		} else if(matchID && match < kMatchId)
		{
			pFound = plug;
			match = kMatchId;
		} else if(matchName && match < kMatchName)
		{
			pFound = plug;
			match = kMatchName;
		}
	}

	if(pFound != nullptr && pFound->Create != nullptr)
	{
		IMixPlugin *plugin = pFound->Create(*pFound, sndFile, mixPlugin);
		if(plugin)
		{
			pFound->InsertPluginInstanceIntoList(*plugin);
		}
#ifdef MODPLUG_TRACKER
		CriticalSection cs;
#endif
		mixPlugin.pMixPlugin = plugin;
		return plugin != nullptr;
	}

#ifdef MODPLUG_TRACKER
	bool maskCrashes = TrackerSettings::Instance().BrokenPluginsWorkaroundVSTMaskAllCrashes;

	if(!pFound && (mixPlugin.GetLibraryName() != U_("")))
	{
		// Try finding the plugin DLL in the plugin directory or plugin cache instead.
		mpt::PathString fullPath = TrackerSettings::Instance().PathPlugins.GetDefaultDir();
		if(fullPath.empty())
		{
			fullPath = theApp.GetInstallPath() + P_("Plugins\\");
		}
		fullPath += mpt::PathString::FromUnicode(mixPlugin.GetLibraryName()) + P_(".dll");

		pFound = AddPlugin(fullPath, maskCrashes);
		if(!pFound)
		{
			// Try plugin cache (search for library name)
			SettingsContainer &cacheFile = theApp.GetPluginCache();
			mpt::ustring IDs = cacheFile.Read<mpt::ustring>(cacheSection, mixPlugin.GetLibraryName(), U_(""));
			if(IDs.length() >= 16)
			{
				fullPath = cacheFile.Read<mpt::PathString>(cacheSection, IDs, P_(""));
				if(!fullPath.empty())
				{
					fullPath = theApp.PathInstallRelativeToAbsolute(fullPath);
					if(mpt::native_fs{}.is_file(fullPath))
					{
						pFound = AddPlugin(fullPath, maskCrashes);
					}
				}
			}
		}
	}

#ifdef MPT_WITH_VST
	if(pFound && mixPlugin.Info.dwPluginId1 == Vst::kEffectMagic)
	{
		Vst::AEffect *pEffect = nullptr;
		HINSTANCE hLibrary = nullptr;
		bool validPlugin = false;

		pEffect = CVstPlugin::LoadPlugin(maskCrashes, *pFound, hLibrary, TrackerSettings::Instance().bridgeAllPlugins ? CVstPlugin::BridgeMode::ForceBridgeWithFallback : CVstPlugin::BridgeMode::Automatic);

		if(pEffect != nullptr && pEffect->dispatcher != nullptr && pEffect->magic == Vst::kEffectMagic)
		{
			GetPluginInformation(maskCrashes, pEffect, *pFound);

			// Update cached information
			pFound->WriteToCache();

			CVstPlugin *pVstPlug = new (std::nothrow) CVstPlugin(maskCrashes, hLibrary, *pFound, mixPlugin, *pEffect, sndFile);
			if(pVstPlug)
			{
				pFound->InsertPluginInstanceIntoList(*pVstPlug);
			}
			validPlugin = (pVstPlug != nullptr);
			CriticalSection cs;
			mixPlugin.pMixPlugin = pVstPlug;
		}

		if(!validPlugin)
		{
			FreeLibrary(hLibrary);
			CVstPluginManager::ReportPlugException(MPT_UFORMAT("Unable to create plugin \"{}\"!\n")(pFound->libraryName));
		}
		return validPlugin;
	} else
	{
		// "plug not found" notification code MOVED to CSoundFile::Create
#ifdef VST_LOG
		MPT_LOG_GLOBAL(LogDebug, "VST", U_("Unknown plugin"));
#endif
	}
#endif // MPT_WITH_VST

#endif // MODPLUG_TRACKER
	return false;
}


#ifdef MODPLUG_TRACKER
void CVstPluginManager::OnIdle()
{
	for(auto &factory : pluginList)
	{
		// Note: bridged plugins won't receive these messages and generate their own idle messages.
		IMixPlugin *p = factory->pPluginsList;
		while (p)
		{
			//rewbs. VSTCompliance: A specific plug has requested indefinite periodic processing time.
			p->Idle();
			//We need to update all open editors
			CAbstractVstEditor *editor = p->GetEditor();
			if (editor && editor->m_hWnd)
			{
				editor->UpdateParamDisplays();
			}
			//end rewbs. VSTCompliance:

			p = p->GetNextInstance();
		}
	}
}


void CVstPluginManager::ReportPlugException(const mpt::ustring &msg)
{
	Reporting::Notification(msg);
#ifdef VST_LOG
	MPT_LOG_GLOBAL(LogDebug, "VST", mpt::ToUnicode(msg));
#endif
}

#endif // MODPLUG_TRACKER

OPENMPT_NAMESPACE_END

#endif // NO_PLUGINS
