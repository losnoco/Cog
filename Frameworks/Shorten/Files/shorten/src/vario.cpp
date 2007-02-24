/*
 *  vario.cpp
 *  shorten_decoder
 *
 *  Created by Alex Lagutin on Tue Jul 09 2002.
 *  Copyright (c) 2002 Eckysoft All rights reserved.
 *
 */

/******************************************************************************
*                                                                             *
*  Copyright (C) 1992-1995 Tony Robinson                                      *
*                                                                             *
*  See the file doc/LICENSE.shorten for conditions on distribution and usage  *
*                                                                             *
******************************************************************************/

/*
 * $Id: vario.c,v 1.6 2001/12/30 05:12:04 jason Exp $
 */

#include "shn_reader.h"

void shn_reader::var_get_init()
{
	int		i;
	ulong	val = 0;
	
	masktab[0]	= val;
	for (i = 1; i < MASKTABSIZE; i++)
	{
		val <<= 1;
		val |= 1;
		masktab[i] = val;
	}

	mDecodeState.getbuf   = (uchar *) malloc(BUFSIZ);
	mDecodeState.getbufp  = mDecodeState.getbuf;
	mDecodeState.nbyteget = 0;
	mDecodeState.gbuffer  = 0;
	mDecodeState.nbitget  = 0;
	
	if (!mDecodeState.getbuf)
		mFatalError	= true;
}

ulong shn_reader::word_get()
{
	ulong	buffer;

	if (mDecodeState.nbyteget < 4)
	{
		mDecodeState.nbyteget	+= fread(mDecodeState.getbuf, 1, BUFSIZ, mFP);
		if (mDecodeState.nbyteget < 4)
		{
			mFatalError	= true;
			return 0;
		}
		mDecodeState.getbufp = mDecodeState.getbuf;
	}

	buffer	= (((long) (mDecodeState.getbufp[0])) << 24) | (((long) (mDecodeState.getbufp[1])) << 16)
			| (((long) (mDecodeState.getbufp[2])) <<  8) | ((long) (mDecodeState.getbufp[3]));

	mDecodeState.getbufp	+= 4;
	mDecodeState.nbyteget	-= 4;
	
	return buffer;
}

long shn_reader::uvar_get(int nbin)
{
	long result;
	
	if (!mDecodeState.nbitget)
	{
		mDecodeState.gbuffer = word_get();
		if (mFatalError)
			return 0;
		mDecodeState.nbitget	= 32;
	}

	for (result = 0; !(mDecodeState.gbuffer & (1L << --(mDecodeState.nbitget))); result++)
	{
		if (!mDecodeState.nbitget)
		{
			mDecodeState.gbuffer	= word_get();
			if (mFatalError)
				return 0;
			mDecodeState.nbitget	= 32;
		}
	}

	while (nbin)
	{
		if(mDecodeState.nbitget >= nbin)
		{
			result = (result << nbin) | ((mDecodeState.gbuffer >> (mDecodeState.nbitget-nbin)) & masktab[nbin]);
			mDecodeState.nbitget	-= nbin;
			nbin = 0;
		}
		else
		{
			result = (result << mDecodeState.nbitget) | (mDecodeState.gbuffer & masktab[mDecodeState.nbitget]);
			mDecodeState.gbuffer = word_get();
			if (mFatalError)
				return 0;
			nbin -= mDecodeState.nbitget;
			mDecodeState.nbitget	= 32;
		}
	}

	return result;
}

ulong shn_reader::ulong_get()
{
	uint nbit = uvar_get(ULONGSIZE);

	if (mFatalError)
		return 0;

	return uvar_get(nbit);
}

long shn_reader::var_get(int nbin)
{
	ulong uvar = uvar_get(nbin + 1);
	if (mFatalError)
		return 0;
	
	return ((uvar & 1) ? ((long) ~(uvar >> 1)) : ((long) (uvar >> 1)));
}

void shn_reader::var_get_quit()
{
	free(mDecodeState.getbuf);
	mDecodeState.getbuf	= NULL;
}
