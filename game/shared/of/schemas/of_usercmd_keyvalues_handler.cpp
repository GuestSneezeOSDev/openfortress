#include "cbase.h"

#include "of_usercmd_keyvalues_handler.h"
#include "filesystem.h"

#include "KeyValues.h"

#include "tier0/memdbgon.h"

const char *g_aValidUserKeyvalueNames[OF_KEYVALUE_MESSAGE_COUNT] =
{
	"LoadoutInfo"
};

bool IsValidUserKeyValuesMessage( const char *szName )
{
	for( int i = 0; i < OF_KEYVALUE_MESSAGE_COUNT; i++ )
	{
		if( !Q_strncasecmp( szName, g_aValidUserKeyvalueNames[i], sizeof(g_aValidUserKeyvalueNames[i]) ) )
			return true;
	}

	return false;
}

#ifdef CLIENT_DLL
//======================================================================================================
// Purpose: Simple class that handles messages a client wants to send to the server
//			Can't same multiple of the same message type
//			Try to keep these to a minimum, they take up quite the bandwidth
//=====================================================================================================

COFUserCMDKeyValueHandler g_UserCMDKeyValueHandler;

// Global function, tells the handler to add a message into its queue
void SendKeyValuesToServer( KeyValues *pKV )
{
	g_UserCMDKeyValueHandler.AddMessage( pKV );
}

void COFUserCMDKeyValueHandler::AddMessage( KeyValues *pKV )
{
	// Remove it if it exists already, aka overwrite it
	m_Messages.Remove( pKV->GetName() );
	
	// TODO: Possibly copy the keyvalues instead of just adding their pointers?
	//		 Only really needed if the Keyvalues we're sending
	//		 are used elsewhere for whatever reason
	//		 In that case the keyvalues would need to be deleted manually
	//		 by the user after being sent here - Kay
	m_Messages.Insert( pKV->GetName(), pKV );
}

void COFUserCMDKeyValueHandler::RemoveMessage( const char *szName )
{
	// Remove and dealloc the keyvalues
	m_Messages.Remove( szName );
}
#endif

//======================================================================================================
// Purpose: The following two functions handle reading and writing keyvalues into a UserCMD buffer
//			They are mostly copies of KeyValues::WriteAsBinary and KeyValues::ReadAsBinary
//			so they shouldn't have any major bugs attached to them
//			
//			Keep in mind that they are FRIEND classes of KeyValues since they use
//			protected members such as m_pPeer, m_sValue and similar
//			So if you need to change their type or inputs, make sure to update them in public/tier1/KeyValues.h too!
//			For further questions ask Kay ^v^
//=====================================================================================================


//======================================================================================================
// Purpose: Write KeyValues to a UserCMD buffer to be sent to the server
//======================================================================================================
void KeyValuesWriteToUserCMD( KeyValues *pKV, bf_write *buffer )
{
	// Write subkeys:
	
	// loop through all our peers
	for ( KeyValues *dat = pKV; dat != NULL; dat = dat->m_pPeer )
	{
		// write type
		buffer->WriteByte(dat->GetDataType());

		// write name
		buffer->WriteString(dat->GetName());

		// write type
		switch( dat->GetDataType() )
		{
		case KeyValues::TYPE_NONE:
			{
				// Forward the new key back to ourselves
				KeyValuesWriteToUserCMD( dat->m_pSub, buffer );
				break;
			}
		case KeyValues::TYPE_STRING:
			{
				// Write a string
				if (dat->m_sValue && *(dat->m_sValue))
				{
					buffer->WriteString( dat->m_sValue );
				}
				else
				{
					buffer->WriteString( "" );
				}
				break;
			}
		case KeyValues::TYPE_WSTRING:
			{
				// We can't support wstrings atm
				Assert( !"TYPE_WSTRING" );
				break;
			}

		case KeyValues::TYPE_INT:
			{
				// Write our int, simple enough
				// TODO: Maybe replace wit write/read byte? - Kay
				buffer->WriteSignedVarInt32( dat->m_iValue );				
				break;
			}

		case KeyValues::TYPE_UINT64:
			{
				// Write our unsigned it, not sure why they do it like this
				// TODO: If it causes issues, we need to look into it more - Kay
				buffer->WriteVarInt64( *((double *)dat->m_sValue) );
				break;
			}

		case KeyValues::TYPE_FLOAT:
			{
				// Write our float, simple enough
				buffer->WriteFloat( dat->m_flValue );
				break;
			}
		case KeyValues::TYPE_COLOR:
			{
				// Have to write each byte individually
				buffer->WriteByte( dat->m_Color[0] );
				buffer->WriteByte( dat->m_Color[1] );
				buffer->WriteByte( dat->m_Color[2] );
				buffer->WriteByte( dat->m_Color[3] );
				break;
			}
		case KeyValues::TYPE_PTR:
			{
				// Write our pointer
				buffer->WriteVarInt32( (int)dat->m_pValue );
			}

		default:
			break;
		}
	}

	// write tail, marks end of peers
	buffer->WriteByte( KeyValues::TYPE_NUMTYPES ); 
}

//======================================================================================================
// Purpose: Read KeyValues from an UserCMD buffer a client is trying to send us
//======================================================================================================
bool KeyValuesReadFromUserCMD( KeyValues *pKv, bf_read *buffer, int nStackDepth )
{
	pKv->RemoveEverything(); // remove current content
	pKv->Init();	// reset
	
	if ( nStackDepth > 100 )
	{
		AssertMsgOnce( false, "KeyValuesReadFromUserCMD stack depth > 100\n" );
		return false;
	}

	KeyValues	*dat = pKv;
	KeyValues::types_t		type = (KeyValues::types_t)buffer->ReadByte();
	
	// loop through all our peers
	while ( true )
	{
		if ( type == KeyValues::TYPE_NUMTYPES )
			break; // no more peers

		dat->m_iDataType = type;

		{
			// Read and allocate our string
			char *token = buffer->ReadAndAllocateString();

			// Set our name
			dat->SetName( token );

			// If its the first stack, that means this is the message name
			// Check to see if the message name is in our message database
			// If it isn't, the client may be trying to send malicious data
			// Bail out and return false, the UserCMD processer will kick them afterwards
			if( nStackDepth == 0 && !IsValidUserKeyValuesMessage( token ) )
			{
				// Deallocate the middleman token since we dont need it anymore
				delete token;
				return false;
			}
			
			// Deallocate the middleman token since we dont need it anymore
			delete token;
		}

		switch ( type )
		{
		case KeyValues::TYPE_NONE:
			{
				// Create a new subkey
				dat->m_pSub = new KeyValues("");
				// Forward the next bit of data to our subkey
				KeyValuesReadFromUserCMD( dat->m_pSub, buffer, nStackDepth + 1 );
				break;
			}
		case KeyValues::TYPE_STRING:
			{
				// Read and allocate our middleman token
				char *token = buffer->ReadAndAllocateString();

				// Copy it into our string value
				int len = Q_strlen( token );
				dat->m_sValue = new char[len + 1];
				Q_memcpy( dat->m_sValue, token, len+1 );

				// Delete the middleman
				delete token;
				break;
			}
		case KeyValues::TYPE_WSTRING:
			{
				// We don't support for wstrings atm
				Assert( !"TYPE_WSTRING" );
				break;
			}

		case KeyValues::TYPE_INT:
			{
				// Read our int
				dat->m_iValue = buffer->ReadSignedVarInt32();
				break;
			}

		case KeyValues::TYPE_UINT64:
			{
				// Read our unsigned int
				// Again, a little weird, but this is how valve is doing it
				dat->m_sValue = new char[sizeof(uint64)];
				*((uint64 *)dat->m_sValue) = buffer->ReadVarInt64();
				break;
			}

		case KeyValues::TYPE_FLOAT:
			{
				// Read our float
				dat->m_flValue = buffer->ReadFloat();
				break;
			}
		case KeyValues::TYPE_COLOR:
			{
				// Have to read the color bytes individually
				dat->m_Color[0] = buffer->ReadByte();
				dat->m_Color[1] = buffer->ReadByte();
				dat->m_Color[2] = buffer->ReadByte();
				dat->m_Color[3] = buffer->ReadByte();
				break;
			}
		case KeyValues::TYPE_PTR:
			{
				// Read our pointer
				dat->m_pValue = (void*)buffer->ReadVarInt32();
			}

		default:
			break;
		}

		// Set our type
		type = (KeyValues::types_t)buffer->ReadByte();

		// If the type is TYPE_NUMTYPES, that means we're done reading
		if ( type == KeyValues::TYPE_NUMTYPES )
			break;

		// new peer follows
		dat->m_pPeer = new KeyValues("");
		dat = dat->m_pPeer;
	}
	
	return true;
}