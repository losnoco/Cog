/*  DecMPA - simple MPEG Audio decoding library.
    Copyright (C) 2002  Hauke Duden

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	For more information look at the file License.txt in this package.

	email: hazard_hd@users.sourceforge.net
*/

#ifndef _DECMPA_H_
#define _DECMPA_H_


/**\file
Specifies the DecMPA API.*/


#ifndef DECMPA_VERSION
	
	/**DecMPA API version number.*/
	#define DECMPA_VERSION 1

#endif

#if defined(_WINDOWS) || defined(_WIN32)

	#ifndef CALLBACK
		#include <windows.h>
	#endif
	#define DECMPA_CC CALLBACK

#else

	#define DECMPA_CC

#endif

/**\def DECMPA_CC
Macro for the calling convention used by DecMPA functions.*/


/**\defgroup GroupFunctions Functions*/

/**\defgroup GroupResults Result Codes
Description of the result codes that are returned by most DecMPA functions.

Codes of the form DECMPA_ERROR_*** represent errors and are all negative.
The other result codes, all >=0, indicate different degrees of success.*/

/**\defgroup GroupParamIDs Configuations Parameters
Description of the possible parameters that can be defined using
#DecMPA_SetParam.
*/

/**\defgroup GroupParamValues Configuration Parameter Values
Description of the value constants that can be assigned to some configuration
parameters.*/

/**\defgroup GroupStructs Structures*/


/**\addtogroup GroupStructs*/
/*@{*/

/**This structure contains pointers to the functions that are used
to read the MP3 data during DecMPA_Decode.*/
typedef struct
{
	/**Pointer to the function that is used to read data. If this pointer
	is NULL, DecMPA will read from stdin and ignore the other callback
	functions.

	@param pContext context pointer that was passed to
		#DecMPA_CreateUsingCallbacks
	@param pBuffer buffer that the data should be stored into
	@param nBytes number of bytes to read
	@return the number of bytes read or -1 if an error occurred. If
		the returned value is less than nBytes it is assumed that the end
		of the data stream was reached.
	
	\note the function that the pointer points to should have the calling
	convention DECMPA_CC, i.e. it should be defined as:
	\code
	long DECMPA_CC MyReadFuncName(void* pContext,void* pBuffer,long nBytes);
	\endcode*/
	long (DECMPA_CC* Read)(void* pContext,void* pBuffer,long nBytes);



	/**Pointer to function that seeks to the specified data stream position.
	This function pointer should be set to NULL if seeking is not supported.
	
	@param pContext context pointer that was passed to
		#DecMPA_CreateUsingCallbacks
	@param DestPos destination position, in bytes.
	@return nonzero if successful, 0 otherwise.
	
	\note the function that the pointer points to should have the calling
	convention DECMPA_CC, i.e. it should be defined as:
	\code
	int DECMPA_CC MySeekFuncName(void* pContext,long DestPos);
	\endcode*/
	int (DECMPA_CC* Seek)(void* pContext,long DestPos);
	


	/**Pointer to a function that returns the length of the data stream.
	This function pointer can be set to NULL if the length cannot be
	determined. Alternatively the function can return -1,

	@param pContext context pointer that was passed to
		#DecMPA_CreateUsingCallbacks
	@return the length, in bytes. -1 if unknown.
	
	\note the function that the pointer points to should have the calling
	convention DECMPA_CC, i.e. it should be defined as:
	\code
	long DECMPA_CC MyGetLengthFuncName(void* pContext);
	\endcode*/
	long (DECMPA_CC* GetLength)(void* pContext);



	/**Pointer to a function that returns the current position in the data
	stream. This function pointer can only be NULL if Read is also NULL.

	@param pContext context pointer that was passed to
		#DecMPA_CreateUsingCallbacks
	@return the current position, in bytes or -1 if an error occurred.
	
	\note the function that the pointer points to should have the calling
	convention DECMPA_CC, i.e. it should be defined as:
	\code
	long DECMPA_CC MyGetPositionFuncName(void* pContext);
	\endcode*/
	long (DECMPA_CC* GetPosition)(void* pContext);

} DecMPA_Callbacks;



/**This structure provides information extracted from an MPEG Audio header.*/
typedef struct
{
	/**The original header data.*/
	unsigned char aRawData[4];

	/**Protection*/
	int bProtection;
	/**Layer*/
	int nLayer;
	/**Version*/
	int nVersion;
	/**Padding*/
	int bPadding;
	/**Frequency index*/
	int nFrequencyIndex;
	/**Frequency in Hz*/
	int nFrequency;
	/**Bitrate index*/
	int nBitRateIndex;
	/**Extended mode*/
	int nExtendedMode;
	/**Mode*/
	int nMode;
	/**Input stereo*/
	int bInputStereo;
	/**MPEG 2.5*/
	int bMPEG25;	

	/**Frame size in bytes*/
	int nFrameSize;
	/**Number of decoded samples per frame. If the data is stereo, a sample
	consists of one value for the left channel and one for the right.*/
	int nDecodedSamplesPerFrame;
	/**Bitrate in Kbps.*/
	int nBitRateKbps;

} DecMPA_MPEGHeader;


/**DecMPA_OutputFormat objects are used to specify the format of the decoded
audio data that is returned by DecMPA_Decode.*/
typedef struct
{
	/**Specifies the type of the data. Can be either DECMPA_OUTPUT_INT16 or
	DECMPA_OUTPUT_FLOAT*/
	int nType;

	/**Specifies the frequency of the data.*/
	int nFrequency;

	/**Specifies the number of channels. Can be 1 for mono or 2 for stereo.*/
	int nChannels;

} DecMPA_OutputFormat;

/*@}*/


/**\addtogroup GroupResults*/
/*@{*/

/**The operation has been successfully completed.*/
#define DECMPA_OK					0

/**The end of the stream was reached and there is no more data to decode.*/
#define DECMPA_END					1

/**An invalid parameter was passed to a function.*/
#define DECMPA_ERROR_PARAM			(-1)

/**The operation is not supported.*/
#define DECMPA_ERROR_UNSUPPORTED	(-2)

/**There is not enough free memory.*/
#define DECMPA_ERROR_MEMORY			(-3)

/**An internal error occurred in the decoding routines.*/
#define DECMPA_ERROR_INTERNAL		(-4)

/**Indicates an error during decoding, which usually means that the input data
is invalid or corrupted.*/
#define DECMPA_ERROR_DECODE			(-5)

/**An error occurred during the reading of new data.*/
#define DECMPA_ERROR_READ			(-6)

/**An error occurred when it was tried to seek to a different position in
the data stream.*/
#define DECMPA_ERROR_SEEK			(-7)

/**An error occurred when opening a file.*/
#define DECMPA_ERROR_OPENFILE		(-8)

/**The decoder is in the wrong state to perform the specified
function.*/
#define DECMPA_ERROR_WRONGSTATE		(-9)

/**The requested resource is not available*/
#define DECMPA_ERROR_NOTAVAILABLE	(-10)

/**The version of the DecMPA library that is used is incompatible:*/
#define DECMPA_ERROR_INCOMPATIBLEVERSION	(-11)


/*@}*/




/**\addtogroup GroupParamIDs*/
/*@{*/

/**Specifies the format DecMPA will use to output the decoded audio samples.
Can be either #DECMPA_OUTPUT_INT16 or #DECMPA_OUTPUT_FLOAT.

Default value: #DECMPA_OUTPUT_INT16.*/
#define DECMPA_PARAM_OUTPUT			0

/**Specifies wether the decoder should load a leading ID3v2 tag and
make it available through #DecMPA_GetID3v2Data. Can be either
#DECMPA_YES or #DECMPA_NO.

Default value: #DECMPA_NO.*/
#define DECMPA_PARAM_PROVIDEID3V2	1

/*@}*/

/**Constant indicating the number of definable parameters.*/
#define DECMPA_PARAMCOUNT			2



/**\addtogroup GroupParamValues*/
/*@{*/

/**\defgroup GroupParamGeneric Generic values*/
/*@{*/

/**Indicates that the specified function is activated.*/
#define DECMPA_YES	1

/**Indicates that the specified function is not activated.*/
#define DECMPA_NO	0

/*@}*/

/**\defgroup GroupParamOutput DECMPA_PARAM_OUTPUT values*/
/*@{*/

/**Indicates that the audio samples are signed 16 bit integers (one per
channel). -32768 is the minimum and 32767 is the maximum value.

\note As of version 0.3.0 DecMPA uses integers internally. That means that
#DECMPA_OUTPUT_INT16 is a little faster than #DECMPA_OUTPUT_FLOAT because the
latter requires an additional conversion.*/
#define DECMPA_OUTPUT_INT16			0

/**Indicates that the audio samples are floats (one per channel).
-1.0 is the minimum and 1.0 is the maximum value.

\note As of version 0.3.0 DecMPA uses integers internally. That means that
#DECMPA_OUTPUT_INT16 is a little faster than #DECMPA_OUTPUT_FLOAT because the
latter requires an additional conversion.*/
#define DECMPA_OUTPUT_FLOAT			1

/*@}*/

/*@}*/




/**\addtogroup GroupFunctions*/
/*@{*/


/**Creates a new decoder that obtains its input data from a file.

If you want to provide the input data in some other customized way, use
#DecMPA_CreateUsingCallbacks instead.

After the decoder is created, additional parameters can be configured with
#DecMPA_SetParam.

@param ppDecoder pointer to a void* variable that receives the address of the
	decoder object.
@param sFilePath path of the file to open
@param APIVersion version number of the DecMPA API. You should always
	pass the constant #DECMPA_VERSION for this parameter.
@return a DecMPA result code (see \ref GroupResults).*/
int DECMPA_CC DecMPA_CreateUsingFile(void** ppDecoder,const char* sFilePath,int APIVersion);



/**Creates a new decoder that obtains its input data using callback functions.

After the decoder is created, additional parameters can be configured with
#DecMPA_SetParam.

@param ppDecoder pointer to a void* variable that receives the address of the
	decoder object.
@param pCallbacks structure containing pointers to the callback functions that
	are used to retrieve the MPEG Audio data.
@param pCallbackContext a pointer that is simply passed to the callback
	functions. It has no meaning to the decoder and is only meant to be used
	to pass arbitrary data to the callback functions.
@param APIVersion version number of the DecMPA API. You should always
	pass the constant #DECMPA_VERSION for this parameter.
@return a DecMPA result code (see \ref GroupResults).*/
int DECMPA_CC DecMPA_CreateUsingCallbacks(void** ppDecoder,const DecMPA_Callbacks* pCallbacks,void* pCallbackContext,int APIVersion);



/**Sets additional DecMPA parameters.

Calling this function is completely optional - if you don't call it
or only call it for some parameters, the remaining parameters will simply keep
their default values.

This function should only be called directly after the decoder was created,
before calling any of the other functions. Otherwise DecMPA_SetParam may return
#DECMPA_ERROR_WRONGSTATE.

@param pDecoder the decoder object
@param ID ID of the parameter to set. See \ref GroupParamIDs.
@param Value the new value for the specified parameter. Which values are possible
	depends of the parameter.
@return a DecMPA result code (see \ref GroupResults).*/
int DECMPA_CC DecMPA_SetParam(void* pDecoder,int ID,long Value);



/**Returns the current value of a DecMPA parameter.

@param pDecoder the decoder object
@param ID ID of the parameter. See \ref GroupParamIDs.
@return the current value of the specified parameter.
@see #DecMPA_SetParam
*/
long DECMPA_CC DecMPA_GetParam(void* pDecoder,int ID);



/**Decodes some data and stores it in the supplied buffer. The buffer is not
necessarily completely filled. Always check the value stored in pBytesDecoded
to find out how much data has been decoded.

The format of the data (frequency and number of channels) can be obtained by
calling #DecMPA_GetOutputFormat <b>after</b> DecMPA_Decode. The format may
change from one call of DecMPA_Decode to the next. Wether the format has
changed can be checked using #DecMPA_OutputFormatChanged.

@param pDecoder the decoder object
@param pBuffer pointer to a buffer that receives the decoded data.
@param nBufferBytes size of the buffer, in bytes
@param pBytesDecoded pointer to a variable that receives the number of bytes
	that were stored in the buffer.
@return a DecMPA result code (see \ref GroupResults).*/
int DECMPA_CC DecMPA_Decode(void* pDecoder,void* pBuffer,long nBufferBytes,long* pBytesDecoded);



/**This is a special version of #DecMPA_Decode that does not actually decode
any MPEG Audio data, but only decodes the header information.

This function is a lot faster than #DecMPA_Decode and can be used
to make a quick run through the file and obtain accurate information about
its properties, like the exact duration or decoded file size.

The function behaves in all ways like #DecMPA_Decode, except that it does not
return any data. In particular, the output format and mpeg audio header
information returned by #DecMPA_GetOutputFormat and #DecMPA_GetMPEGHeader will
be properly updated, just as #DecMPA_Decode does.

@param pDecoder the decoder object
@param pDecodedBytes pointer to a variable that receives the decoded size
	(in bytes) of the current MPEG audio frame. If #DecMPA_Decode was called
	before and some	parts of the current frame have already been read, the
	size of the remaining data is returned
@return a DecMPA result code (see \ref GroupResults).*/
int DECMPA_CC DecMPA_DecodeNoData(void* pDecoder,long* pDecodedBytes);



/**Changes the decoding position to the specified time, relative to the
beginning of the data stream.

If #DecMPA_CreateUsingCallbacks was used to create the decoder and no
Seek callback function has been specified, seeking is not supported and
#DECMPA_ERROR_UNSUPPORTED is returned.

@param pDecoder the decoder object
@param Millis the target time, in milliseconds
@return a DecMPA result code (see \ref GroupResults).*/
int DECMPA_CC DecMPA_SeekToTime(void* pDecoder,long Millis);



/**Retrieves the current decoding time. The decoding time corresponds to
the time that it takes to play the data that has been decoded up to now.

@param pDecoder the decoder object
@param pTime pointer to a variable that receives the time, in milliseconds.
@return a DecMPA result code (see \ref GroupResults).*/
int DECMPA_CC DecMPA_GetTime(void* pDecoder,long* pTime);



/**Retrieves the time that it takes to play back the whole data stream.
If the duration is not known -1 is returned.

\note The duration is an estimation that can be slightly wrong for some files
but is pretty accurate for the wide majority of files. If you need a
100% accurate duration value, there is really no other way than to read through
the whole file with #DecMPA_Decode or #DecMPA_DecodeNoData and add up the
\c DecodedBytes values that are returned by those functions. Note that
if you only make this pass through the file to get the duration, i.e. you do
not need decoded audio data, you can use #DecMPA_DecodeNoData which is a lot
faster than #DecMPA_Decode.

@param pDecoder the decoder object
@param pDuration pointer to a variable that receives the duration, in
	milliseconds.
@return a DecMPA result code (see \ref GroupResults).*/
int DECMPA_CC DecMPA_GetDuration(void* pDecoder,long* pDuration);



/**Retrieves the format of the data that was returned by the last call to
DecMPA_Decode.

The nType member of DecMPA_OutputFormat never changes and its value is
defined by the #DECMPA_PARAM_OUTPUT parameter that can be set using
#DecMPA_SetParam. If the value has not been explicitly changed using
#DecMPA_SetParam it will be #DECMPA_OUTPUT_INT16, indicating 16 bit signed
samples.

The remaining fields of the structure may change after calls to
#DecMPA_Decode or #DecMPA_DecodeNoData.

@param pDecoder the decoder object
@param pFormat pointer to a DecMPA_OutputFormat object that is filled with
	the format data
@return a DecMPA result code (see \ref GroupResults).*/
int DECMPA_CC DecMPA_GetOutputFormat(void* pDecoder,DecMPA_OutputFormat* pFormat);



/**Checks wether the output format has changed during the last call to
DecMPA_Decode.

@return nonzero if the format has changed, zero otherwise.*/
int DECMPA_CC DecMPA_OutputFormatChanged(void* pDecoder);



/**Retrieve mpeg header of the data that was returned by the last call to
DecMPA_Decode. This function is only supplied for information purposes.

@param pDecoder the decoder object
@param pHeader pointer to a DecMPA_MPEGHeader object that received the data.
@return a DecMPA result code (see \ref GroupResults).*/
int DECMPA_CC DecMPA_GetMPEGHeader(void* pDecoder,DecMPA_MPEGHeader* pHeader);



/**Returns the file's ID3v2 tag, if it has one. If the file has no ID3v2
tag, #DECMPA_ERROR_NOTAVAILABLE is returned.

This function will always return #DECMPA_ERROR_NOTAVAILABLE if the decoder parameter
#DECMPA_PARAM_PROVIDEID3V2 has not been set to #DECMPA_YES with #DecMPA_SetParam.

\note There are other libraries that can be used to parse the returned
data, one of them being id3lib. It can be found at
http://id3lib.sourceforge.net/ .

@param pDecoder the decoder object
@param ppData pointer to a pointer variable that will receive the address
	of the ID3v2 data. The memory that contains the data will remain valid
	until the decoder is destroyed.
@param pDataSize pointer to a variable that will receive the size of the
	ID3v2 data, in bytes.
@return a DecMPA result code (see \ref GroupResults).*/
int DECMPA_CC DecMPA_GetID3v2Data(void* pDecoder,unsigned char** ppData,long* pDataSize);



/**Destroys a decoder.

@param pDecoder decoder object.*/
void DECMPA_CC DecMPA_Destroy(void* pDecoder);


/**Returns the version number of the DecMPA API used by the library.*/
int DECMPA_CC DecMPA_GetVersion(void);

/*@}*/

#endif


