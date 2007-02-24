/***************************************************************************************
Analyze - Sample 1
Copyright (C) 2000-2001 by Matthew T. Ashland   All Rights Reserved.
Feel free to use this code in any way that you like.

This example opens an APE file and displays some basic information about it. To use it,
just type Sample 1.exe followed by a file name and it'll display information about that
file.

Notes for use in a new project:
	-you need to include "MACLib.lib" in the included libraries list
	-life will be easier if you set the [MAC SDK]\\Shared directory as an include 
	directory and an additional library input path in the project settings
	-set the runtime library to "Mutlithreaded"

WARNING:
	-This class driven system for using Monkey's Audio is still in development, so
	I can't make any guarantees that the classes and libraries won't change before
	everything gets finalized.  Use them at your own risk.
***************************************************************************************/

// includes
#include <stdio.h>

#include "All.h"
#include "GlobalFunctions.h"
#include "MACLib.h"
#include "CharacterHelper.h"
#include "APETag.h"

int main(int argc, char* argv[]) 
{
	///////////////////////////////////////////////////////////////////////////////
	// error check the command line parameters
	///////////////////////////////////////////////////////////////////////////////
	if (argc != 2) 
	{
		printf("~~~Improper Usage~~~\r\n\r\n");
		printf("Usage Example: Sample 1.exe 'c:\\1.ape'\r\n\r\n");
		return 0;
	}

	///////////////////////////////////////////////////////////////////////////////
	// variable declares
	///////////////////////////////////////////////////////////////////////////////
	int					nRetVal = 0;										// generic holder for return values
	char				cTempBuffer[256]; ZeroMemory(&cTempBuffer[0], 256);	// generic buffer for string stuff
	char *				pFilename = argv[1];								// the file to open
	IAPEDecompress *	pAPEDecompress = NULL;								// APE interface
	CSmartPtr<wchar_t> spInput;

	spInput.Assign(GetUTF16FromANSI(argv[1]), TRUE);
	
//*
	///////////////////////////////////////////////////////////////////////////////
	// open the file and error check
	///////////////////////////////////////////////////////////////////////////////
	pAPEDecompress = CreateIAPEDecompress(spInput, &nRetVal);
	if (pAPEDecompress == NULL) 
	{
		printf("Error opening APE file. (error code %d)\r\n\r\n", nRetVal);
		return 0;
	}


	///////////////////////////////////////////////////////////////////////////////
	// display some information about the file
	///////////////////////////////////////////////////////////////////////////////
	printf("Displaying information about '%s':\r\n\r\n", pFilename);

	// file format information
	printf("File Format:\r\n");
	printf("\tVersion: %.2f\r\n", float(pAPEDecompress->GetInfo(APE_INFO_FILE_VERSION)) / float(1000));
	switch (pAPEDecompress->GetInfo(APE_INFO_COMPRESSION_LEVEL))
	{
		case COMPRESSION_LEVEL_FAST: printf("\tCompression level: Fast\r\n\r\n"); break;
		case COMPRESSION_LEVEL_NORMAL: printf("\tCompression level: Normal\r\n\r\n"); break;
		case COMPRESSION_LEVEL_HIGH: printf("\tCompression level: High\r\n\r\n"); break;
		case COMPRESSION_LEVEL_EXTRA_HIGH: printf("\tCompression level: Extra High\r\n\r\n"); break;
	}

	// audio format information
	printf("Audio Format:\r\n");
	printf("\tSamples per second: %d\r\n", pAPEDecompress->GetInfo(APE_INFO_SAMPLE_RATE));
	printf("\tBits per sample: %d\r\n", pAPEDecompress->GetInfo(APE_INFO_BITS_PER_SAMPLE));
	printf("\tNumber of channels: %d\r\n", pAPEDecompress->GetInfo(APE_INFO_CHANNELS));
	printf("\tPeak level: %d\r\n\r\n", pAPEDecompress->GetInfo(APE_INFO_PEAK_LEVEL));

	// size and duration information
	printf("Size and Duration:\r\n");
	printf("\tLength of file (s): %d\r\n", pAPEDecompress->GetInfo(APE_INFO_LENGTH_MS) / 1000);
	printf("\tFile Size (kb): %d\r\n\r\n", pAPEDecompress->GetInfo(APE_INFO_APE_TOTAL_BYTES) / 1024);
	
	// tag information
	printf("Tag Information:\r\n");
	
	CAPETag * pAPETag = (CAPETag *) pAPEDecompress->GetInfo(APE_INFO_TAG);
	BOOL bHasID3Tag = pAPETag->GetHasID3Tag();
	BOOL bHasAPETag = pAPETag->GetHasAPETag();

	
	if (bHasID3Tag || bHasAPETag)
	{
	    printf("\tID3 Tag: %s, APE Tag: %s", bHasID3Tag ? "Yes" : "No", bHasAPETag ? "" : "No");
	    if (bHasAPETag)
	    {
		printf("%d", pAPETag->GetAPETagVersion() / 1000);
	    }
	    printf("\n\n");
		// iterate through all the tag fields
		BOOL bFirst = TRUE;
		CAPETagField * pTagField;
//		while (pAPETag->GetNextTagField(bFirst, &pTagField))
		int index = 0;
		while ((pTagField = pAPETag->GetTagField(index)) != NULL)
		{
			bFirst = FALSE;
			index ++;
			
			// output the tag field properties (don't output huge fields like images, etc.)
			if (pTagField->GetFieldValueSize() > 128)
			{
				printf("\t%s: --- too much data to display ---\r\n", GetANSIFromUTF16(pTagField->GetFieldName()));
			}
			else
			{
/*
			    const wchar_t *fieldName;
			    char *name;
			    wchar_t fieldValue[255];
			    char *value;

			    fieldName = pTagField->GetFieldName();
			    name = GetANSIFromUTF16(fieldName);

			    memset(fieldValue, 0, 255 * sizeof(wchar_t));
			    int len;
			    pAPETag->GetFieldString(fieldName, fieldValue, &len);
			    
			    value = GetANSIFromUTF16(fieldValue);
*/
			    const wchar_t *fieldName;
			    char *name;
			    const char *fieldValue;
			    char *value;

			    fieldName = pTagField->GetFieldName();
			    name = GetANSIFromUTF16(fieldName);

			    fieldValue = pTagField->GetFieldValue();
			    if (pAPETag->GetAPETagVersion() == CURRENT_APE_TAG_VERSION)
			    {
				value = GetANSIFromUTF8((unsigned char *)fieldValue);
			    }
			    else
			    {
				value = (char *)fieldValue;
			    }
			    printf("\t%s : %s\n", name, value);
			}
		}
	}
	else 
	{
		printf("\tNot tagged\r\n\r\n");
	}
	
	///////////////////////////////////////////////////////////////////////////////
	// cleanup (just delete the object
	///////////////////////////////////////////////////////////////////////////////
	delete pAPEDecompress;

	///////////////////////////////////////////////////////////////////////////////
	// quit
	///////////////////////////////////////////////////////////////////////////////
	return 0;
}
