#ifndef YU_CONTAINER_TYPE_H
#define YU_CONTAINER_TYPE_H

#include "platform.h"
#include "new.h"
#include "string.h"

namespace yu
{
	struct true_type { typedef true_type type; enum { value = 1 }; };
	struct false_type{ typedef false_type type; enum { value = 0 }; };

	template<class T>
	struct pod_type : true_type
	{
		void construct(T* ptr)
		{
		
		}

		void construct(T* ptr, size_t num)
		{
			
		}

		void destruct(T* ptr)
		{

		}

		void destruct(T* ptr, size_t num)
		{

		}
	};

	template<class T>
	struct not_pod_type : false_type
	{
		void construct(T* ptr)
		{
			new(ptr) T;
		}

		void construct(T* ptr, size_t num)
		{
			for(size_t i = 0; i < num; i++)
			{
				construct(ptr+i);
			}
		}

		void destruct(T* ptr)
		{
			ptr->~T();
		}

		void destruct(T* ptr, size_t num)
		{
			for(size_t i = 0; i < num; i++)
			{
				destruct(ptr + i);
			}
		}
	};

	template<class TYPE> struct is_pod : not_pod_type<TYPE> {  };
	template<class TYPE> struct is_pod<TYPE*> : pod_type<TYPE*> {  };
	template<> struct is_pod<char>              : pod_type<char> {  };
	template<> struct is_pod<unsigned char>     : pod_type<unsigned char> {  };
	template<> struct is_pod<short>             : pod_type<short> {  };
	template<> struct is_pod<unsigned short>    : pod_type<unsigned short> {  };
	template<> struct is_pod<int>               : pod_type<int> {  };
	template<> struct is_pod<unsigned int>      : pod_type<unsigned int> {  };
	template<> struct is_pod<long>              : pod_type<long> {  };	
	template<> struct is_pod<unsigned long>     : pod_type<unsigned long> {  };	
	template<> struct is_pod<long long>         : pod_type<long long> {  };	
	template<> struct is_pod<unsigned long long>: pod_type<unsigned long long> {  };	
	template<> struct is_pod<float>             : pod_type<float>  {  };	
	template<> struct is_pod<double>            : pod_type<double> {  };

	template<class TYPE> struct is_pod<const TYPE> : is_pod<TYPE>::type {  };	 // what the fuck, all the ridiculous shit can do with template... 
	template<class T> struct is_pod<T[]> : is_pod<T>::type {};
	template<class T, size_t n> struct is_pod<T[n]> : is_pod<T>::type {};


	//template<class CLASS> struct is_mem_copyable : false_type {};
	template<class CLASS> struct is_mem_copyable : is_pod<CLASS>::type {}; 

	template<class IS_MEM_COPYABLE_T1, class IS_MEM_COPYABLE_T2> struct IS_MEM_COPYABLE2 : false_type {};
	template<> struct IS_MEM_COPYABLE2<true_type, true_type> : true_type {};
	template<class T1, class T2> struct is_mem_copyable2 : IS_MEM_COPYABLE2< typename is_mem_copyable<T1>::type, typename is_mem_copyable<T2>::type >::type {};


	//---------------------------find type of object & pointer-----------------------------------
	template<class T> struct type_of { typedef T type; };
	template<class T> struct type_of<T[]> { typedef typename type_of<T>::type type; };	
	template<class T, size_t n> struct type_of<T[n]> { typedef typename type_of<T>::type type; };


	template<class POINTER_TYPE> struct type_of_pointer{ };	
	template<class T> struct type_of_pointer<T*>             { typedef T type; };
	template<class T> struct type_of_pointer<const T*>       { typedef T type; };
	template<class T> struct type_of_pointer<T* const>       { typedef T type; };
	template<class T> struct type_of_pointer<const T* const> { typedef T type; };	

	template<class POINTER_TYPE>
	struct type_of_raw_pointer{};
	template<class T> struct type_of_raw_pointer<T*>             { typedef T* type; };
	template<class T> struct type_of_raw_pointer<const T*>       { typedef T* type; };
	template<class T> struct type_of_raw_pointer<T* const>       { typedef T* type; };
	template<class T> struct type_of_raw_pointer<const T* const> { typedef T* type; };


	template<class T>
	void Construct(T* p)
	{
		is_pod<T> type;
		type.construct(p);
	}

	template<class T>
	void Construct(T* p, size_t num)
	{
		is_pod<T> type;
		type.construct(p, num);
	}

	template<class T>
	void Destruct(T* p)
	{
		is_pod<T> type;
		type.destruct(p);
	}

	template<class T>
	void Destruct(T* p, size_t num)
	{
		is_pod<T> type;
		type.destruct(p, num);
	}

	//-------------------------------------copy  stuff------------------------------------------------------

	template<class sIterator, class dIterator>
	YU_INLINE void do_copy_n(sIterator source, size_t count, dIterator dest, true_type)
	{
		yu::memcpy(dest, source, sizeof( typename type_of_pointer<sIterator>::type )  * count );
	}

	template<class sIterator, class dIterator>
	YU_INLINE void do_copy_n(sIterator source, size_t count, dIterator dest, false_type)
	{
		for(size_t i = 0; i < count; i++ ){
			*(dest++) =  *(source++);
		}
	}		

	//------------------------optimized copy, when type is pod, use memcpy, else use loop----------------
	template<class sIterator, class dIterator>
	YU_INLINE void copy_n(sIterator source, size_t count, dIterator dest)
	{
		do_copy_n(source, count, dest, typename is_mem_copyable2< typename type_of_pointer<sIterator>::type, typename type_of_pointer<dIterator>::type >::type() );
	}

	template<class sIterator, class dIterator>
	YU_INLINE void do_copy(sIterator sourceBegin, sIterator sourceEnd, dIterator dest, true_type)
	{
		yu::memcpy(dest, sourceBegin, (size_t) sourceEnd - (size_t) sourceBegin);
	}

	template<class sIterator, class dIterator>
	YU_INLINE void do_copy(sIterator sourceBegin, sIterator sourceEnd, dIterator dest, false_type)
	{
		while( sourceBegin!= sourceEnd ){
			*(dest++) = *(sourceBegin++);
		}

	}

	template<class sIterator, class dIterator>
	YU_INLINE void copy(sIterator sourceBegin, sIterator sourceEnd, dIterator dest)
	{
		do_copy(sourceBegin, sourceEnd, dest, typename is_mem_copyable2< typename type_of_pointer<sIterator>::type, typename type_of_pointer<dIterator>::type >::type() );
	}

}

#endif