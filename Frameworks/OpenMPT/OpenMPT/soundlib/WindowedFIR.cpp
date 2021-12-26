/*
 * WindowedFIR.cpp
 * ---------------
 * Purpose: FIR resampling code
 * Notes  : Original code from modplug-xmms
 * Authors: OpenMPT Devs
 *          ModPlug-XMMS Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "WindowedFIR.h"
#include "mpt/base/numbers.hpp"
#include <cmath>

OPENMPT_NAMESPACE_BEGIN

double CWindowedFIR::coef( int _PCnr, double _POfs, double _PCut, int _PWidth, int _PType ) //float _PPos, float _PFc, int _PLen )
{
	const double epsilon = 1e-8;
	const double _LWidthM1 = _PWidth - 1;
	const double _LWidthM1Half = 0.5 * _LWidthM1;
	const double _LPosU = (_PCnr - _POfs);
	const double _LPIdl = (2.0 * mpt::numbers::pi) / _LWidthM1;
	double _LPos = _LPosU - _LWidthM1Half;
	double _LWc, _LSi;
	if(std::abs(_LPos) < epsilon)
	{
		_LWc = 1.0;
		_LSi = _PCut;
	} else
	{
		switch(_PType)
		{
		case WFIR_HANN:
			_LWc = 0.50 - 0.50 * std::cos(_LPIdl * _LPosU);
			break;
		case WFIR_HAMMING:
			_LWc = 0.54 - 0.46 * std::cos(_LPIdl * _LPosU);
			break;
		case WFIR_BLACKMANEXACT:
			_LWc = 0.42 - 0.50 * std::cos(_LPIdl * _LPosU) + 0.08 * std::cos(2.0 * _LPIdl * _LPosU);
			break;
		case WFIR_BLACKMAN3T61:
			_LWc = 0.44959 - 0.49364 * std::cos(_LPIdl * _LPosU) + 0.05677 * std::cos(2.0 * _LPIdl * _LPosU);
			break;
		case WFIR_BLACKMAN3T67:
			_LWc = 0.42323 - 0.49755 * std::cos(_LPIdl * _LPosU) + 0.07922 * std::cos(2.0 * _LPIdl * _LPosU);
			break;
		case WFIR_BLACKMAN4T92: // blackman harris
			_LWc = 0.35875 - 0.48829 * std::cos(_LPIdl * _LPosU) + 0.14128 * std::cos(2.0 * _LPIdl * _LPosU) - 0.01168 * std::cos(3.0 * _LPIdl * _LPosU);
			break;
		case WFIR_BLACKMAN4T74:
			_LWc = 0.40217 - 0.49703 * std::cos(_LPIdl * _LPosU) + 0.09392 * std::cos(2.0 * _LPIdl * _LPosU) - 0.00183 * std::cos(3.0 * _LPIdl * _LPosU);
			break;
		case WFIR_KAISER4T: // kaiser-bessel, alpha~7.5
			_LWc = 0.40243 - 0.49804 * std::cos(_LPIdl * _LPosU) + 0.09831 * std::cos(2.0 * _LPIdl * _LPosU) - 0.00122 * std::cos(3.0 * _LPIdl * _LPosU);
			break;
		default:
			_LWc = 1.0;
			break;
		}
		_LPos *= mpt::numbers::pi;
		_LSi = std::sin(_PCut * _LPos) / _LPos;
	}
	return (_LWc * _LSi);
}

void CWindowedFIR::InitTable(double WFIRCutoff, uint8 WFIRType)
{
	const double _LPcllen = (double)(1 << WFIR_FRACBITS);  // number of precalculated lines for 0..1 (-1..0)
	const double _LNorm = 1.0 / (2.0 * _LPcllen);
	const double _LCut = WFIRCutoff;
	for(int _LPcl = 0; _LPcl < WFIR_LUTLEN; _LPcl++)
	{
		double _LGain = 0.0, _LCoefs[WFIR_WIDTH];
		const double _LOfs = (_LPcl - _LPcllen) * _LNorm;
		const int _LIdx = _LPcl << WFIR_LOG2WIDTH;
		for(int _LCc = 0; _LCc < WFIR_WIDTH; _LCc++)
		{
			_LGain += (_LCoefs[_LCc] = coef(_LCc, _LOfs, _LCut, WFIR_WIDTH, WFIRType));
		}
		_LGain = 1.0 / _LGain;
		for(int _LCc = 0; _LCc < WFIR_WIDTH; _LCc++)
		{
#ifdef MPT_INTMIXER
			double _LCoef = std::floor(0.5 + WFIR_QUANTSCALE * _LCoefs[_LCc] * _LGain);
			lut[_LIdx + _LCc] = (signed short)((_LCoef < -WFIR_QUANTSCALE) ? -WFIR_QUANTSCALE : ((_LCoef > WFIR_QUANTSCALE) ? WFIR_QUANTSCALE : _LCoef));
#else
			double _LCoef = _LCoefs[_LCc] * _LGain;
			lut[_LIdx + _LCc] = (float)_LCoef;
#endif // MPT_INTMIXER
		}
	}
}


OPENMPT_NAMESPACE_END
