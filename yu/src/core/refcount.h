#ifndef YU_REFCOUNT_H
#define YU_REFCOUNT_H

namespace yu
{
class RefCounted
{
public:
	RefCounted() : refcount(0) {}
	int    IncrementRef() { return ++refcount; }
	int    DecrementRef() { return --refcount; }
	int    RefCount() { return refcount; }
protected:
	int refcount;
};

template<class T>
class RefCountedPtr 
{
public:
	RefCountedPtr();
	~RefCountedPtr();
	RefCountedPtr(T* _ptr);
	RefCountedPtr(const RefCountedPtr<T>& _rhs);

	RefCountedPtr& operator=(T* _ptr);
	RefCountedPtr& operator=(const RefCountedPtr<T>& _rhs);
	operator T*  () const  ;
	operator const T* () const;
	T& operator* () const;
	T* operator->();
	const T* operator->() const;

	operator bool() const;
	bool operator !() const;
	bool operator ==(const T* _ptr) const;
	bool operator ==(T* _ptr) const;
	bool operator ==(const RefCountedPtr<T>& _rhs) const;
	bool operator !=(const T* _ptr) const;
	bool operator !=(T* _ptr) const;
	bool operator !=(const RefCountedPtr& _rhs) const;
	bool operator <(const T* _ptr) const;
	bool operator >(const T* _ptr) const;
	void Swap(RefCountedPtr<T>& rhs);

	T* Get() const;

private:
	T* ptr;

};

template<class T> inline RefCountedPtr<T>::RefCountedPtr() : ptr(nullptr)
{

}

template<class T> inline RefCountedPtr<T>::~RefCountedPtr()
{
	if(ptr && ptr->DecrementRef() == 0)
	{
		delete ptr;
	}
}

template<class T> inline RefCountedPtr<T>::RefCountedPtr(T* _ptr)
{
	ptr = _ptr;
	if(ptr)
		ptr->IncrementRef();
}

template<class T> inline RefCountedPtr<T>::RefCountedPtr(const RefCountedPtr<T>& _rhs)
{	
	ptr = _rhs.ptr;
	if(ptr)
		ptr->IncrementRef();
}

template<class T> inline RefCountedPtr<T>& RefCountedPtr<T>::operator=(T *_ptr)
{
	if(_ptr)
		_ptr->IncrementRef();
	if(ptr)
		ptr->DecrementRef();

	ptr = _ptr;
	return *this;
}

template<class T> inline RefCountedPtr<T>& RefCountedPtr<T>::operator=(const RefCountedPtr<T>& _rhs)
{
	if(_rhs.ptr)
		_rhs.ptr->IncrementRef();
	if(ptr)
		ptr->DecrementRef();

	ptr = _rhs.ptr;
	return *this;
}


template<class T> inline RefCountedPtr<T>::operator T*() const
{
	return ptr;
}

template<class T> inline RefCountedPtr<T>::operator const T *() const
{
	return ptr;
}

template<class T> inline T& RefCountedPtr<T>::operator *() const
{
	return *ptr;
}

template<class T> inline T* RefCountedPtr<T>::operator ->()
{
	return ptr;
}

template<class T> inline const T* RefCountedPtr<T>::operator ->() const
{
	return ptr;
}

template<class T> inline RefCountedPtr<T>::operator bool() const
{
	return ptr != nullptr;
}

template<class T> inline bool RefCountedPtr<T>::operator !() const
{
	return ptr == nullptr;
}

template<class T> inline bool RefCountedPtr<T>::operator ==(const T *_ptr) const
{
	return ptr == _ptr;
}

template<class T> inline bool RefCountedPtr<T>::operator ==(T *_ptr) const
{
	return ptr = _ptr;
}

template<class T> inline bool RefCountedPtr<T>::operator !=(const T *_ptr) const
{
	return ptr != _ptr;
}

template<class T> inline bool RefCountedPtr<T>::operator !=(T *_ptr) const
{
	return ptr != _ptr;
}

template<class T> inline bool RefCountedPtr<T>::operator !=(const RefCountedPtr<T> &_rhs) const
{
	return ptr != _rhs.m_ptr;
}

template<class T> inline bool RefCountedPtr<T>::operator <(const T *_ptr) const
{
	return ptr < _ptr;
}

template<class T> inline bool RefCountedPtr<T>::operator >(const T *_ptr) const
{
	return ptr > _ptr;
}

template<class T> inline void RefCountedPtr<T>::Swap(RefCountedPtr<T>& _rhs)
{
	T tmp = ptr;
	ptr = _rhs;
	_rhs = tmp;
}

template<class T> inline T* RefCountedPtr<T>::Get() const
{
	return ptr;
}
}

#endif