#ifndef _CDATAPOOL_H
#define _CDATAPOOL_H

#include "carray.h"

//-----------------------------------------------------------------------------

template < class _T > 
class CDataPool {
// wii edit: _U defined as a constant somewhere...
	template < class _1U > 
	class CPoolElem {
		public:
			int	prev;
			int	next;
			_1U	data;
		};

	class CPoolBuffer : public CArray< CPoolElem <_T> > {};

	private:
		CPoolBuffer		m_buffer;
		//_T*				m_null;
		int				m_free;
		int				m_used;

	public:
		CDataPool () { Init (); }
		~CDataPool() { Destroy (); }

		//_T& operator[] (uint i) { return m_buffer [i].data; }

		inline void Init (void) { 
			m_buffer.Init ();
			m_free = 
			m_used = -1; 
			}

		inline void Destroy (void) { 
			m_buffer.Destroy (); 
			Init (); 
			}

		inline bool Create (uint size) { 
			Destroy ();
			if (!m_buffer.Create (size))
				return false;
			uint i;
			for (i = 0; i < size; i++) {
				m_buffer [i].prev = i - 1;
				m_buffer [i].next = i + 1;
				}
			m_buffer [i - 1].next = -1;
			m_free = 0;
			m_used = -1;
			return true;
			}

		inline _T* Pop (void) { 
			if (m_free < 0)
				return NULL;
			CPoolElem<_T>& e = m_buffer [m_free];
			int next = e.next;
			e.prev = -1;
			e.next = m_used;
			if (m_used >= 0)
				m_buffer [m_used].prev = m_free;
			m_used = m_free;
			m_free = next;
			return &e.data; 
			}

		void Push (uint i) {
			if (m_used < 0)
				return;
			CPoolElem<_T>& e = m_buffer [i];
			if (e.prev < 0)
				m_used = e.next;
			else
				m_buffer [e.prev].next = e.next;
			if (e.next >= 0)
				m_buffer [e.next].prev = e.prev;
			e.prev = -1;
			e.next = m_free;
			if (m_free >= 0)
				m_buffer [m_free].prev = i;
			m_free = i;
			}

		inline int LastIndex (void) { return m_used; }

		inline _T* GetNext (int& nCurrent) { 
			if (nCurrent < 0)
				return NULL;
			CPoolElem<_T>* e;
			try {
				e = &m_buffer [nCurrent]; 
				}
			catch(...) {
#if DBG
				ArrayError ("Data pool element chain broken.");
#endif
				return NULL;
				}
			nCurrent = e->next;
			return &e->data;
			}

		inline _T* GetFirst (int& nCurrent) { 
			if (nCurrent < 0)
				nCurrent = m_used;
			return GetNext (nCurrent);
			}

		inline int Size (void) { return m_buffer.Size (); }

		inline int UsedList (void) { return m_used; }
		inline int FreeList (void) { return m_free; }

		inline _T& operator[] (uint i) { return m_buffer [i].data; }
		inline _T* operator+ (uint i) { return &m_buffer [i].data; }
	};


//-----------------------------------------------------------------------------

#endif //_CDATAPOOL_H
