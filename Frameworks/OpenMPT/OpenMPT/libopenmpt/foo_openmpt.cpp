
#if defined(_MSC_VER)
#pragma warning(disable:4091)
#endif

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4996)
#endif
#include "foobar2000/SDK/foobar2000.h"
#include "foobar2000/helpers/helpers.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include "libopenmpt.hpp"

#include <algorithm>
#include <locale>
#include <string>
#include <vector>



// Declaration of your component's version information
// Since foobar2000 v1.0 having at least one of these in your DLL is mandatory to let the troubleshooter tell different versions of your component apart.
// Note that it is possible to declare multiple components within one DLL, but it's strongly recommended to keep only one declaration per DLL.
// As for 1.1, the version numbers are used by the component update finder to find updates; for that to work, you must have ONLY ONE declaration per DLL. If there are multiple declarations, the component is assumed to be outdated and a version number of "0" is assumed, to overwrite the component with whatever is currently on the site assuming that it comes with proper version numbers.
DECLARE_COMPONENT_VERSION("OpenMPT component", OPENMPT_API_VERSION_STRING ,"libopenmpt based module file player");



// This will prevent users from renaming your component around (important for proper troubleshooter behaviors) or loading multiple instances of it.
VALIDATE_COMPONENT_FILENAME("foo_openmpt.dll");


// settings


// {FFD659CA-6AEA-479f-8E60-F03B297286FE}
static const GUID guid_openmpt_root = 
{ 0xffd659ca, 0x6aea, 0x479f, { 0x8e, 0x60, 0xf0, 0x3b, 0x29, 0x72, 0x86, 0xfe } };


// {E698E101-FD93-4e6c-AF02-AEC7E8631D8E}
static const GUID guid_openmpt_samplerate = 
{ 0xe698e101, 0xfd93, 0x4e6c, { 0xaf, 0x2, 0xae, 0xc7, 0xe8, 0x63, 0x1d, 0x8e } };

// {E4026686-02F9-4805-A3FD-2EECA655A92C}
static const GUID guid_openmpt_channels = 
{ 0xe4026686, 0x2f9, 0x4805, { 0xa3, 0xfd, 0x2e, 0xec, 0xa6, 0x55, 0xa9, 0x2c } };

// {7C845F81-9BA3-4a9a-9C94-C7056DFD1B57}
static const GUID guid_openmpt_gain = 
{ 0x7c845f81, 0x9ba3, 0x4a9a, { 0x9c, 0x94, 0xc7, 0x5, 0x6d, 0xfd, 0x1b, 0x57 } };

// {EDB0E1E5-BD2E-475c-B2FB-8448C92F6F29}
static const GUID guid_openmpt_stereo = 
{ 0xedb0e1e5, 0xbd2e, 0x475c, { 0xb2, 0xfb, 0x84, 0x48, 0xc9, 0x2f, 0x6f, 0x29 } };

// {9115A820-67F5-4d0a-B0FB-D312F7FBBAFF}
static const GUID guid_openmpt_repeat = 
{ 0x9115a820, 0x67f5, 0x4d0a, { 0xb0, 0xfb, 0xd3, 0x12, 0xf7, 0xfb, 0xba, 0xff } };

// {EAAD5E60-F75B-4071-B308-9956362ECB69}
static const GUID guid_openmpt_filter = 
{ 0xeaad5e60, 0xf75b, 0x4071, { 0xb3, 0x8, 0x99, 0x56, 0x36, 0x2e, 0xcb, 0x69 } };

// {0CF7E156-44E3-4587-A727-864B045BEDDD}
static const GUID guid_openmpt_amiga =
{ 0x0cf7e156, 0x44e3, 0x4587,{ 0xa7, 027, 0x86, 0x4b, 0x04, 0x5b, 0xed, 0xdd } };

// {497BF503-D825-4A02-BE5C-02DB8911B1DC}
static const GUID guid_openmpt_ramping = 
{ 0x497bf503, 0xd825, 0x4a02, { 0xbe, 0x5c, 0x2, 0xdb, 0x89, 0x11, 0xb1, 0xdc } };


static advconfig_branch_factory g_advconfigBranch("OpenMPT Component", guid_openmpt_root, advconfig_branch::guid_branch_decoding, 0);

static advconfig_integer_factory   cfg_samplerate("Samplerate [6000..96000] (Hz)"                                     , guid_openmpt_samplerate, guid_openmpt_root, 0, 48000, 6000, 96000);
static advconfig_integer_factory   cfg_channels  ("Channels [1=mono, 2=stereo, 4=quad]"                               , guid_openmpt_channels  , guid_openmpt_root, 0,     2,    1,     4);
static advconfig_string_factory_MT cfg_gain      ("Gain [-12...12] (dB)"                                              , guid_openmpt_gain      , guid_openmpt_root, 0, "0.0");
static advconfig_integer_factory   cfg_stereo    ("Stereo separation [0...200] (%)"                                   , guid_openmpt_stereo    , guid_openmpt_root, 0,   100,    0,   200);
static advconfig_string_factory_MT cfg_repeat    ("Repeat [0=never, -1=forever, 1..] (#)"                             , guid_openmpt_repeat    , guid_openmpt_root, 0,   "0");
static advconfig_integer_factory   cfg_filter    ("Interpolation filter length [1=nearest, 2=linear, 4=cubic, 8=sinc]", guid_openmpt_filter    , guid_openmpt_root, 0,     8,    1,     8);
static advconfig_checkbox_factory  cfg_amiga     ("Use Amiga Resampler for Amiga modules"                             , guid_openmpt_amiga,      guid_openmpt_root, 0, false);
static advconfig_string_factory_MT cfg_ramping   ("Volume ramping [-1=default, 0=off, 1..10] (ms)"                    , guid_openmpt_ramping   , guid_openmpt_root, 0,  "-1");

struct foo_openmpt_settings {
	int samplerate;
	int channels;
	int mastergain_millibel;
	int stereoseparation;
	int repeatcount;
	int interpolationfilterlength;
	int ramping;
	bool use_amiga_resampler;
	foo_openmpt_settings() {

		/*
		samplerate = 48000;
		channels = 2;
		mastergain_millibel = 0;
		stereoseparation = 100;
		repeatcount = 0;
		interpolationfilterlength = 8;
		use_amiga_resampler = false;
		ramping = -1;
		*/

		pfc::string8 tmp;
		
		samplerate = static_cast<int>( cfg_samplerate.get() );
		
		channels = static_cast<int>( cfg_channels.get() );
		if ( channels == 3 ) {
			channels = 2;
		}
		
		cfg_gain.get( tmp );
		mastergain_millibel = static_cast<int>( pfc::string_to_float( tmp ) * 100.0 );
		
		stereoseparation = static_cast<int>( cfg_stereo.get() );
		
		cfg_repeat.get( tmp );
		repeatcount = static_cast<int>( pfc::atoi64_ex( tmp, ~0 ) );
		if ( repeatcount < -1 ) {
			repeatcount = 0;
		}
		
		interpolationfilterlength = static_cast<int>( cfg_filter.get() );

		use_amiga_resampler = cfg_amiga.get();
		
		cfg_ramping.get( tmp );
		ramping = static_cast<int>( pfc::atoi64_ex( tmp, ~0 ) );
		if ( ramping < -1 ) {
			ramping = -1;
		}

	}
};



// Note that input class does *not* implement virtual methods or derive from interface classes.
// Our methods get called over input framework templates. See input_singletrack_impl for descriptions of what each method does.
// input_stubs just provides stub implementations of mundane methods that are irrelevant for most implementations.
class input_openmpt : public input_stubs {
public:
	void open(service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort) {
		if ( p_reason == input_open_info_write ) {
			throw exception_io_unsupported_format(); // our input does not support retagging.
		}
		m_file = p_filehint; // p_filehint may be null, hence next line
		input_open_file_helper(m_file,p_path,p_reason,p_abort); // if m_file is null, opens file with appropriate privileges for our operation (read/write for writing tags, read-only otherwise).
		if ( m_file->get_size( p_abort ) >= (std::numeric_limits<std::size_t>::max)() ) {
			throw exception_io_unsupported_format();
		}
		std::vector<char> data( static_cast<std::size_t>( m_file->get_size( p_abort ) ) );
		m_file->read( data.data(), data.size(), p_abort );
		try {
			std::map< std::string, std::string > ctls;
			ctls["seek.sync_samples"] = "1";
			mod = new openmpt::module( data, std::clog, ctls );
		} catch ( std::exception & /*e*/ ) {
			throw exception_io_data();
		}
		settings = foo_openmpt_settings();
		mod->set_repeat_count( settings.repeatcount );
		mod->set_render_param( openmpt::module::RENDER_MASTERGAIN_MILLIBEL, settings.mastergain_millibel );
		mod->set_render_param( openmpt::module::RENDER_STEREOSEPARATION_PERCENT, settings.stereoseparation );
		mod->set_render_param( openmpt::module::RENDER_INTERPOLATIONFILTER_LENGTH, settings.interpolationfilterlength );
		mod->set_render_param( openmpt::module::RENDER_VOLUMERAMPING_STRENGTH, settings.ramping );
		mod->ctl_set( "render.resampler.emulate_amiga", settings.use_amiga_resampler ? "1" : "0" );
	}
	void get_info(file_info & p_info,abort_callback & p_abort) {
		p_info.set_length( mod->get_duration_seconds() );
		p_info.info_set_int( "samplerate", settings.samplerate );
		p_info.info_set_int( "channels", settings.channels );
		p_info.info_set_int( "bitspersample", 32 );
		std::vector<std::string> keys = mod->get_metadata_keys();
		for ( std::vector<std::string>::iterator key = keys.begin(); key != keys.end(); ++key ) {
			if ( *key == "message_raw" ) {
				continue;
			}
			p_info.meta_set( (*key).c_str(), mod->get_metadata( *key ).c_str() );
		}
	}
	t_filestats get_file_stats(abort_callback & p_abort) {
		return m_file->get_stats(p_abort);
	}
	void decode_initialize(unsigned p_flags,abort_callback & p_abort) {
		m_file->reopen(p_abort); // equivalent to seek to zero, except it also works on nonseekable streams
	}
	bool decode_run(audio_chunk & p_chunk,abort_callback & p_abort) {
		if ( settings.channels == 1 ) {

			std::size_t count = mod->read( settings.samplerate, buffersize, left.data() );
			if ( count == 0 ) {
				return false;
			}
			for ( std::size_t frame = 0; frame < count; frame++ ) {
				buffer[frame*1+0] = left[frame];
			}
			p_chunk.set_data_32( buffer.data(), count, settings.channels, settings.samplerate );
			return true;

		} else if ( settings.channels == 2 ) {

			std::size_t count = mod->read( settings.samplerate, buffersize, left.data(), right.data() );
			if ( count == 0 ) {
				return false;
			}
			for ( std::size_t frame = 0; frame < count; frame++ ) {
				buffer[frame*2+0] = left[frame];
				buffer[frame*2+1] = right[frame];
			}
			p_chunk.set_data_32( buffer.data(), count, settings.channels, settings.samplerate );
			return true;

		} else if ( settings.channels == 4 ) {

			std::size_t count = mod->read( settings.samplerate, buffersize, left.data(), right.data(), rear_left.data(), rear_right.data() );
			if ( count == 0 ) {
				return false;
			}
			for ( std::size_t frame = 0; frame < count; frame++ ) {
				buffer[frame*4+0] = left[frame];
				buffer[frame*4+1] = right[frame];
				buffer[frame*4+2] = rear_left[frame];
				buffer[frame*4+3] = rear_right[frame];
			}
			p_chunk.set_data_32( buffer.data(), count, settings.channels, settings.samplerate );
			return true;

		} else {
			return false;
		}

	}
	void decode_seek(double p_seconds,abort_callback & p_abort) {
		mod->set_position_seconds( p_seconds );
	}
	bool decode_can_seek() {
		return true;
	}
	bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta) { // deals with dynamic information such as VBR bitrates
		return false;
	}
	bool decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) { // deals with dynamic information such as track changes in live streams
		return false;
	}
	void decode_on_idle(abort_callback & p_abort) {
		m_file->on_idle( p_abort );
	}
	void retag(const file_info & p_info,abort_callback & p_abort) {
		throw exception_io_unsupported_format();
	}
	static bool g_is_our_content_type(const char * p_content_type) { // match against supported mime types here
		return false;
	}
	static bool g_is_our_path(const char * p_path,const char * p_extension) {
		if ( !p_extension ) {
			return false;
		}
		std::vector<std::string> extensions = openmpt::get_supported_extensions();
		std::string ext = p_extension;
		std::transform( ext.begin(), ext.end(), ext.begin(), tolower );
		return std::find( extensions.begin(), extensions.end(), ext ) != extensions.end();
	}
	static GUID g_get_guid() {
		// {B0B7CCC3-4520-44D3-B5F9-22EB9EBA7575}
		static const GUID foo_openmpt_guid = { 0xb0b7ccc3, 0x4520, 0x44d3, { 0xb5, 0xf9, 0x22, 0xeb, 0x9e, 0xba, 0x75, 0x75 } };
		return foo_openmpt_guid;
	}
	static const char * g_get_name() {
		return "OpenMPT Module Decoder";
	}
private:
	service_ptr_t<file> m_file;
	static const std::size_t buffersize = 1024;
	foo_openmpt_settings settings;
	openmpt::module * mod;
	std::vector<float> left;
	std::vector<float> right;
	std::vector<float> rear_left;
	std::vector<float> rear_right;
	std::vector<float> buffer;
public:
	input_openmpt() : mod(0), left(buffersize), right(buffersize), rear_left(buffersize), rear_right(buffersize), buffer(4*buffersize) {}
	~input_openmpt() { delete mod; mod = 0; }
};

static input_singletrack_factory_t<input_openmpt> g_input_openmpt_factory;


class input_file_type_v2_impl_openmpt : public input_file_type_v2 {
public:
	input_file_type_v2_impl_openmpt()
		: extensions( openmpt::get_supported_extensions() ) 
	{ }
	unsigned get_count() {
		return static_cast<unsigned>( extensions.size() );
	}
	bool is_associatable( unsigned idx ) {
		return true;
	}
	void get_format_name( unsigned idx, pfc::string_base & out, bool isPlural ) {
		if ( isPlural ) {
			out = "OpenMPT compatible module files";
		} else {
			out = "OpenMPT compatible module file";	
		}
	}
	void get_extensions( unsigned idx, pfc::string_base & out ) {
		out = extensions[idx].c_str();
	}
private:
	std::vector<std::string> extensions;
};

namespace { static service_factory_single_t<input_file_type_v2_impl_openmpt> g_filetypes; }
