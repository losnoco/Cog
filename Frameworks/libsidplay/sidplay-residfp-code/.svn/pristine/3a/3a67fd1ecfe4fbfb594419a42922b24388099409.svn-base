/* Simple smart pointer class. */

#ifndef SMARTPTR_H
#define SMARTPTR_H

#include "sidcxx11.h"

namespace libsidplayfp
{

typedef unsigned long int ulint_smartpt;

template<class T>
class SmartPtrBase_sidtt
{
public:
    SmartPtrBase_sidtt(T* buffer, ulint_smartpt bufferLen, bool bufOwner = false) :
        dummy(0)
	{
		doFree = bufOwner;
        if (bufferLen)
		{
			bufBegin = buffer;
			pBufCurrent = buffer;
			bufEnd = bufBegin + bufferLen;
			bufLen = bufferLen;
			status = true;
		}
		else
		{
            bufBegin = nullptr;
            pBufCurrent = nullptr;
            bufEnd = nullptr;
			bufLen = 0;
			status = false;
		}
	}

	virtual ~SmartPtrBase_sidtt()
	{
        if (doFree && bufBegin != nullptr)
        {
            delete[] bufBegin;
		}
	}

	/* --- public member functions --- */

	virtual T* tellBegin() const { return bufBegin; }
	virtual ulint_smartpt tellLength() const { return bufLen; }
    virtual ulint_smartpt tellPos() const { return static_cast<ulint_smartpt>(pBufCurrent - bufBegin); }

    virtual bool checkIndex(ulint_smartpt index) const
	{
        return (pBufCurrent + index) < bufEnd;
	}

	virtual bool reset()
	{
        if (bufLen)
		{
			pBufCurrent = bufBegin;
			return (status = true);
		}
		else
		{
			return (status = false);
		}
	}

    virtual bool good() const
	{
        return pBufCurrent < bufEnd;
	}

    virtual bool fail() const
	{
        return pBufCurrent == bufEnd;
	}

	virtual void operator ++()
	{
        if (good())
		{
			pBufCurrent++;
		}
		else
		{
			status = false;
		}
	}

	virtual void operator ++(int)
	{
        if (good())
		{
			pBufCurrent++;
		}
		else
		{
			status = false;
		}
	}

	virtual void operator --()
	{
        if (!fail())
		{
			pBufCurrent--;
		}
		else
		{
			status = false;
		}
	}

	virtual void operator --(int)
	{
        if (!fail())
		{
			pBufCurrent--;
		}
		else
		{
			status = false;
		}
	}

	virtual void operator +=(ulint_smartpt offset)
	{
		if (checkIndex(offset))
		{
			pBufCurrent += offset;
		}
		else
		{
			status = false;
		}
	}

	virtual void operator -=(ulint_smartpt offset)
	{
		if ((pBufCurrent-offset) >= bufBegin)
		{
			pBufCurrent -= offset;
		}
		else
		{
			status = false;
		}
	}

	virtual T operator*()
	{
        if (good())
		{
			return *pBufCurrent;
		}
		else
		{
			status = false;
			return dummy;
		}
	}

	virtual T& operator [](ulint_smartpt index)
	{
		if (checkIndex(index))
		{
			return pBufCurrent[index];
		}
		else
		{
			status = false;
			return dummy;
		}
	}

    virtual operator bool() { return status; }

 protected:
	T* bufBegin;
	T* bufEnd;
	T* pBufCurrent;
	ulint_smartpt bufLen;
	bool status;
	bool doFree;
	T dummy;
};


template<class T>
class SmartPtr_sidtt final : public SmartPtrBase_sidtt<T>
{
public:
    SmartPtr_sidtt(T* buffer, ulint_smartpt bufferLen, bool bufOwner = false) :
        SmartPtrBase_sidtt<T>(buffer, bufferLen, bufOwner)
    {}

    SmartPtr_sidtt() :
        SmartPtrBase_sidtt<T>(0, 0)
    {}

	void setBuffer(T* buffer, ulint_smartpt bufferLen)
	{
        if (bufferLen)
		{
			this->bufBegin = buffer;
			this->pBufCurrent = buffer;
			this->bufEnd = buffer + bufferLen;
			this->bufLen = bufferLen;
			this->status = true;
		}
		else
		{
            this->bufBegin = nullptr;
            this->pBufCurrent = nullptr;
            this->bufEnd = nullptr;
			this->bufLen = 0;
			this->status = false;
		}
	}
};

}

#endif  /* SMARTPTR_H */
