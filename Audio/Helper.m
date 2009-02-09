/*
 *  Helper.c
 *  CogAudio
 *
 *  Created by Andre Reffhaug on 2/17/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include <math.h>
#include "Helper.h"

//These functions are helpers for the process of converting volume from a linear to logarithmic scale.
//Numbers that goes in to audioPlayer should be logarithmic. Numbers that are displayed to the user should be linear.
//Here's why: http://www.dr-lex.34sp.com/info-stuff/volumecontrols.html
//We are using the approximation of X^4.
//Input/Output values are in percents.
double logarithmicToLinear(double logarithmic)
{
	return pow((logarithmic/MAX_VOLUME), 0.25) * 100.0;
}

double linearToLogarithmic(double linear)
{
	return (linear/100.0) * (linear/100.0) * (linear/100.0) * (linear/100.0) * MAX_VOLUME;
}
//End helper volume function thingies. ONWARDS TO GLORY!
