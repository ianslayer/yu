#ifndef YU_RESOURCE_PTR_DX_H
#define YU_RESOURCE_PTR_DX_H
namespace yu
{
template<class T>
struct DxResourcePtr
{
public:
	DxResourcePtr() : ptr(nullptr) {}
	~DxResourcePtr();
	DxResourcePtr(T* _ptr);


	DxResourcePtr(const DxResourcePtr& _rhs);

	DxResourcePtr& operator=(T* _ptr);
	DxResourcePtr& operator=(const DxResourcePtr<T>& _rhs);
	operator T*  () const;
	operator const T* () const;
	T& operator* () const;
	T* operator->();
	const T* operator->() const;

	operator bool() const;
	bool operator !() const;
	bool operator ==(const T* _ptr) const;
	bool operator ==(T* _ptr) const;
	bool operator ==(const DxResourcePtr<T>& _rhs) const;
	bool operator !=(const T* _ptr) const;
	bool operator !=(T* _ptr) const;
	bool operator !=(const DxResourcePtr& _rhs) const;

	T* ptr;
};

template<class T> inline DxResourcePtr<T>::~DxResourcePtr()
{
	int refCount;
	if (ptr)
	{
		refCount = ptr->Release();
	}
}

template<class T> inline DxResourcePtr<T>::DxResourcePtr(T* _ptr)
{
	//int refCount;
	//if (ptr)
	//refCount = ptr->Release();

	ptr = _ptr;
}

template<class T> inline DxResourcePtr<T>::DxResourcePtr(const DxResourcePtr<T>& _rhs)
{
	int refCount;
	ptr = _rhs.ptr;
	if (ptr)
		refCount = ptr->AddRef();
}

template<class T> inline DxResourcePtr<T>& DxResourcePtr<T>::operator=(T *_ptr)
{
	int refCount;
	//if (_ptr)
	//	refCount = _ptr->AddRef();
	if (_ptr == ptr)
		return *this;

	if (ptr)
		refCount = ptr->Release();

	ptr = _ptr;
	return *this;
}

template<class T> inline DxResourcePtr<T>& DxResourcePtr<T>::operator=(const DxResourcePtr<T>& _rhs)
{
	int refCount;
	if (_rhs.ptr)
		refCount = _rhs.ptr->AddRef();
	if (ptr)
		refCount = ptr->Release();

	ptr = _rhs.ptr;
	return *this;
}


template<class T> inline DxResourcePtr<T>::operator T*() const
{
	return ptr;
}

template<class T> inline DxResourcePtr<T>::operator const T *() const
{
	return ptr;
}

template<class T> inline T& DxResourcePtr<T>::operator *() const
{
	return *ptr;
}

template<class T> inline T* DxResourcePtr<T>::operator ->()
{
	return ptr;
}

template<class T> inline const T* DxResourcePtr<T>::operator ->() const
{
	return ptr;
}

template<class T> inline DxResourcePtr<T>::operator bool() const
{
	return ptr != nullptr;
}

template<class T> inline bool DxResourcePtr<T>::operator !() const
{
	return ptr == nullptr;
}

template<class T> inline bool DxResourcePtr<T>::operator ==(const T *_ptr) const
{
	return ptr == _ptr;
}

template<class T> inline bool DxResourcePtr<T>::operator ==(T *_ptr) const
{
	return ptr = _ptr;
}

template<class T> inline bool DxResourcePtr<T>::operator !=(const T *_ptr) const
{
	return ptr != _ptr;
}

template<class T> inline bool DxResourcePtr<T>::operator !=(T *_ptr) const
{
	return ptr != _ptr;
}

template<class T> inline bool DxResourcePtr<T>::operator !=(const DxResourcePtr<T> &_rhs) const
{
	return ptr != _rhs.m_ptr;
}


}

#endif