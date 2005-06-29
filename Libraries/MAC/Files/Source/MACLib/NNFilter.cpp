#include "All.h"
#include "GlobalFunctions.h"
#include "NNFilter.h"
//#include <altivec.h>

CNNFilter::CNNFilter(int nOrder, int nShift, int nVersion)
{
    if ((nOrder <= 0) || ((nOrder % 16) != 0)) throw(1);
    m_nOrder = nOrder;
    m_nShift = nShift;
    m_nVersion = nVersion;
    
	//m_bMMXAvailable = GetMMXAvailable();
	m_AltiVecAvailable = IsAltiVecAvailable();

    m_rbInput.Create(NN_WINDOW_ELEMENTS, m_nOrder);
    m_rbDeltaM.Create(NN_WINDOW_ELEMENTS, m_nOrder);
    m_paryM = new short [m_nOrder];

#ifdef NN_TEST_MMX
    srand(GetTickCount());
#endif
}

CNNFilter::~CNNFilter()
{
    SAFE_ARRAY_DELETE(m_paryM)
}

void CNNFilter::Flush()
{
    memset(&m_paryM[0], 0, m_nOrder * sizeof(short));
    m_rbInput.Flush();
    m_rbDeltaM.Flush();
    m_nRunningAverage = 0;
}

int CNNFilter::Compress(int nInput)
{
    // convert the input to a short and store it
    m_rbInput[0] = GetSaturatedShortFromInt(nInput);

    // figure a dot product
	int nDotProduct;
	/*
    if (m_bMMXAvailable)
		nDotProduct = CalculateDotProduct(&m_rbInput[-m_nOrder], &m_paryM[0], m_nOrder);
	*/
	if(m_AltiVecAvailable)
		nDotProduct = CalculateDotProductAltiVec(&m_rbInput[-m_nOrder], &m_paryM[0], m_nOrder);
    else
        nDotProduct = CalculateDotProductNoMMX(&m_rbInput[-m_nOrder], &m_paryM[0], m_nOrder);

    // calculate the output
    int nOutput = nInput - ((nDotProduct + (1 << (m_nShift - 1))) >> m_nShift);

	// adapt
	/*
    if (m_bMMXAvailable)
        Adapt(&m_paryM[0], &m_rbDeltaM[-m_nOrder], -nOutput, m_nOrder);
	*/
	if(m_AltiVecAvailable)
		AdaptAltiVec(&m_paryM[0], &m_rbDeltaM[-m_nOrder], nOutput, m_nOrder);
	else
        AdaptNoMMX(&m_paryM[0], &m_rbDeltaM[-m_nOrder], nOutput, m_nOrder);

    int nTempABS = abs(nInput);

    if (nTempABS > (m_nRunningAverage * 3))
        m_rbDeltaM[0] = ((nInput >> 25) & 64) - 32;
    else if (nTempABS > (m_nRunningAverage * 4) / 3)
        m_rbDeltaM[0] = ((nInput >> 26) & 32) - 16;
    else if (nTempABS > 0)
        m_rbDeltaM[0] = ((nInput >> 27) & 16) - 8;
    else
        m_rbDeltaM[0] = 0;

    m_nRunningAverage += (nTempABS - m_nRunningAverage) / 16;

    m_rbDeltaM[-1] >>= 1;
    m_rbDeltaM[-2] >>= 1;
    m_rbDeltaM[-8] >>= 1;
        
    // increment and roll if necessary
    m_rbInput.IncrementSafe();
    m_rbDeltaM.IncrementSafe();

    return nOutput;
}

int CNNFilter::Decompress(int nInput)
{
    // figure a dot product
    int nDotProduct;
	
	/*
    if (m_bMMXAvailable)
        nDotProduct = CalculateDotProduct(&m_rbInput[-m_nOrder], &m_paryM[0], m_nOrder);
	*/
	if(m_AltiVecAvailable)
		nDotProduct = CalculateDotProductAltiVec(&m_rbInput[-m_nOrder], &m_paryM[0], m_nOrder);
	else
        nDotProduct = CalculateDotProductNoMMX(&m_rbInput[-m_nOrder], &m_paryM[0], m_nOrder);
    
	// adapt
	/*
    if (m_bMMXAvailable)
        Adapt(&m_paryM[0], &m_rbDeltaM[-m_nOrder], -nInput, m_nOrder);
	*/
	if(m_AltiVecAvailable)
		AdaptAltiVec(&m_paryM[0], &m_rbDeltaM[-m_nOrder], nInput, m_nOrder);
	else
        AdaptNoMMX(&m_paryM[0], &m_rbDeltaM[-m_nOrder], nInput, m_nOrder);

    // store the output value
    int nOutput = nInput + ((nDotProduct + (1 << (m_nShift - 1))) >> m_nShift);

    // update the input buffer
    m_rbInput[0] = GetSaturatedShortFromInt(nOutput);

    if (m_nVersion >= 3980)
    {
        int nTempABS = abs(nOutput);

        if (nTempABS > (m_nRunningAverage * 3))
            m_rbDeltaM[0] = ((nOutput >> 25) & 64) - 32;
        else if (nTempABS > (m_nRunningAverage * 4) / 3)
            m_rbDeltaM[0] = ((nOutput >> 26) & 32) - 16;
        else if (nTempABS > 0)
            m_rbDeltaM[0] = ((nOutput >> 27) & 16) - 8;
        else
            m_rbDeltaM[0] = 0;

        m_nRunningAverage += (nTempABS - m_nRunningAverage) / 16;

        m_rbDeltaM[-1] >>= 1;
        m_rbDeltaM[-2] >>= 1;
        m_rbDeltaM[-8] >>= 1;
    }
    else
    {
        m_rbDeltaM[0] = (nOutput == 0) ? 0 : ((nOutput >> 28) & 8) - 4;
        m_rbDeltaM[-4] >>= 1;
        m_rbDeltaM[-8] >>= 1;
    }

    // increment and roll if necessary
    m_rbInput.IncrementSafe();
    m_rbDeltaM.IncrementSafe();
    
    return nOutput;
}

void CNNFilter::AdaptAltiVec(short * pM, short * pAdapt, int nDirection, int nOrder)
{
	vector signed short LSQ2, LSQ4, v1, v2;
	vector unsigned char mask2;
	
	nOrder >>= 4;
	
	//mask1 = vec_lvsl(0,pM);
	mask2 = vec_lvsl(0,pAdapt);
	//align = vec_lvsr(0,pM);
	//zero = (vector unsigned char)(0);
	//(vector signed char) one = (vector signed char)(-1);
	//mask3 = vec_perm((vector unsigned char)(0),(vector unsigned char)(-1),align);
	
	//LSQ3 = vec_ld(0,pM);
	LSQ4 = vec_ld(0,pAdapt);
	
	if (nDirection < 0) 
	{
		while (nOrder--)
		{
			
			v1 = vec_ld(0,pM);
			LSQ2 = vec_ld(16,pAdapt);
			v2 = vec_perm(LSQ4,LSQ2,mask2);
			v1 = vec_add(v1,v2);
			vec_st(v1,0,pM);
			
			/*
			v1 = vec_perm(v1,v1,align);
			LSQ3 = vec_sel(LSQ3,v1,(vector unsigned short)mask3);
			vec_st(LSQ3,0,pM);
			LSQ4 = vec_sel(v1,LSQ,(vector unsigned short)mask3);
			vec_st(LSQ4,16,pM);
			*/
			
			v1 = vec_ld(16,pM);
			LSQ4 = vec_ld(32,pAdapt);
			v2 = vec_perm(LSQ2,LSQ4,mask2);
			v1 = vec_add(v1,v2);
			vec_st(v1,16,pM);
			
			/*
			v1 = vec_perm(v1,v1,align);
			LSQ = vec_sel(LSQ,v1,(vector unsigned short)mask3);
			vec_st(LSQ,16,pM);
			LSQ2 = vec_sel(v1,LSQ3,(vector unsigned short)mask3);
			vec_st(LSQ2,32,pM);
			*/
			
			//memcpy(pM,buffer,32);
			pM = pM + 16;
			pAdapt = pAdapt + 16;
		}
	}
	else if (nDirection > 0)
	{
		while (nOrder--)
		{
			
			v1 = vec_ld(0,pM);
			LSQ2 = vec_ld(16,pAdapt);
			v2 = vec_perm(LSQ4,LSQ2,mask2);
			v1 = vec_sub(v1,v2);
			vec_st(v1,0,pM);
			
			/*
			v1 = vec_perm(v1,v1,align);
			LSQ3 = vec_sel(LSQ3,v1,(vector unsigned short)mask3);
			vec_st(LSQ3,0,pM);
			LSQ4 = vec_sel(v1,LSQ,(vector unsigned short)mask3);
			vec_st(LSQ4,16,pM);
			*/
			
			v1 = vec_ld(16,pM);
			LSQ4 = vec_ld(32,pAdapt);
			v2 = vec_perm(LSQ2,LSQ4,mask2);
			v1 = vec_sub(v1,v2);
			vec_st(v1,16,pM);
			
			/*
			v1 = vec_perm(v1,v1,align);
			LSQ = vec_sel(LSQ,v1,(vector unsigned short)mask3);
			vec_st(LSQ,16,pM);
			LSQ2 = vec_sel(v1,LSQ3,(vector unsigned short)mask3);
			vec_st(LSQ2,32,pM);
			*/
			
			//memcpy(pM,buffer,32);
			pM = pM + 16;
			pAdapt = pAdapt + 16;
			
			
		}
	}
}

int CNNFilter::CalculateDotProductAltiVec(short * pA, short * pB, int nOrder)
{
	vector signed short LSQ, LSQ3, v1, v2;
	vector unsigned char mask1;
	
	vector signed int vzero = (vector signed int)(0);
	vector signed int sum = (vector signed int)(0);
	//sum = vec_xor(sum,sum);
	
	//int nDotProduct;
	int p[4];
	nOrder >>= 4;
	
	mask1 = vec_lvsl(0,pA);
	//mask2 = vec_lvsl(0,pB);
	
	
	LSQ3 = vec_ld(0, pA);
	//LSQ4 = vec_ld(0, pB);
	
	while (nOrder--)
	{
		
		LSQ = vec_ld(16,pA);
		v1 = vec_perm(LSQ3,LSQ,mask1);
		v2 = vec_ld(0,pB);
		sum = vec_msum(v1,v2,sum);
		
		LSQ3 = vec_ld(32,pA);
		v1 = vec_perm(LSQ,LSQ3,mask1);
		v2 = vec_ld(16,pB);
		sum = vec_msum(v1,v2,sum);
		
		pA = pA + 16;
		pB = pB + 16;
		
	}
	
	sum = vec_sums(sum,vzero);
	vec_st(sum,0,p); 
	
	return p[3];
	
}

void CNNFilter::AdaptNoMMX(short * pM, short * pAdapt, int nDirection, int nOrder)
{
    nOrder >>= 4;

    if (nDirection < 0) 
    {    
        while (nOrder--)
        {
            EXPAND_16_TIMES(*pM++ += *pAdapt++;)
        }
    }
    else if (nDirection > 0)
    {
        while (nOrder--)
        {
            EXPAND_16_TIMES(*pM++ -= *pAdapt++;)
        }
    }
}

int CNNFilter::CalculateDotProductNoMMX(short * pA, short * pB, int nOrder)
{
    int nDotProduct = 0;
    nOrder >>= 4;

    while (nOrder--)
    {
        EXPAND_16_TIMES(nDotProduct += *pA++ * *pB++;)
    }
    
    return nDotProduct;
}
