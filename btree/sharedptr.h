#ifndef PTR_H_
#define PTR_H_

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>


template <typename T>
class SharedPtrVal
{
public:
	SharedPtrVal<T>(T pVal) :
		m_pVal(pVal),
		m_nRefCount(1)
	{
	}

	virtual ~SharedPtrVal<T>() 
	{
	}

	void ref()
	{
		m_nRefCount++;
	}

	void deref()
	{
		m_nRefCount--;
		if (m_nRefCount <= 0)
		{
			delete m_pVal;
		}
	}

	T val() const
	{
		return m_pVal;
	}

private:
	SharedPtrVal<T>() {}

private:
	T m_pVal;
	int m_nRefCount;
};



template <typename T>
class SharedPtr {

public:
	SharedPtr<T>(T pVal) :
		m_ptr(pVal)
	{
	}

	SharedPtr<T>(const SharedPtr<T> &o) :
		m_ptr(o.m_ptr)
	{
		m_ptr.ref();
	}

	T val() const
	{
		return m_ptr.val();
	}

	virtual ~SharedPtr<T>()
	{
		m_ptr.deref();
	}

private:
	SharedPtr<T>() {}

private:

	SharedPtrVal<T> m_ptr;
};


int main(int argc, char **argv)
{
	char *y = (char *)"hello";
	SharedPtr<char *> x = y;	
	printf("%s\n", x.val());
}


#endif // PTR_H_

