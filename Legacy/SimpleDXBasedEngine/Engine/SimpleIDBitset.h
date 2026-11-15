#ifndef __SIMPLE_ID_BITSET_H
#define __SIMPLE_ID_BITSET_H

#include <assert.h>
#include "StringHash.h"

namespace Engine
{

	class SimpleIDBitset
	{
		struct _TrackedID
		{
			bool			m_InUse;
			StringHash		m_Hash;
		} ;

		struct _TrackedID	m_IDs[32];

	public:
		SimpleIDBitset()
		{
			for( unsigned int i = 0; i < 32; i++ )
				m_IDs[i].m_InUse = false;
		}

		unsigned int GetID( const StringHash & i_HashedString )
		{
			int firstFree = -1;

			for( unsigned int i = 0; i < 32; i++ )
			{
				if( !m_IDs[i].m_InUse  &&  (firstFree == -1) )
					firstFree = i;
				else if( m_IDs[i].m_InUse  &&  m_IDs[i].m_Hash == i_HashedString )
					return static_cast<unsigned int>( i );
			}

			assert( firstFree != -1 );

			m_IDs[firstFree].m_InUse = true;
			m_IDs[firstFree].m_Hash = i_HashedString;

			return static_cast<unsigned int>( firstFree );
		}

		void ReleaseID( unsigned int i_ID )
		{
			assert( i_ID < 32 );

			m_IDs[i_ID].m_InUse = false;
		}

		void ReleaseID( const StringHash & i_HashedString )
		{
			for( unsigned int i = 0; i < 32; i++ )
				if( m_IDs[i].m_Hash == i_HashedString )
				{
					m_IDs[i].m_InUse = false;
					break;
				}
		}

		static unsigned int IDtoBitmask( unsigned int i_ID )
		{
			assert( i_ID < 32 );

			return 1 << i_ID;
		}
	} ;
}
#endif // __SIMPLE_ID_BITSET_H
