#ifndef PLUGINUSERMESSAGES_HPP_
#define PLUGINUSERMESSAGES_HPP_ 

#include <irecipientfilter.h>

class CRecipientFilter : public IRecipientFilter
{
public:
	CRecipientFilter()
	{
		Reset();
	}

	virtual ~CRecipientFilter() {};

	virtual bool IsReliable(void) const override
	{
		return m_bReliable;
	}

	virtual bool IsInitMessage(void) const override
	{
		return m_bInitMessage;
	}

	virtual int GetRecipientCount(void) const override
	{
		return m_Recipients.Size();
	}

	virtual int GetRecipientIndex(int slot) const override
	{
		if (slot < 0 || slot >= GetRecipientCount())
			return -1;

		return m_Recipients[slot];
	}

	void MakeReliable(void)
	{
		m_bReliable = true;
	}

	void MakeInitMessage(void)
	{
		m_bInitMessage = true;
	}

public:

	void Reset(void)
	{
		m_bReliable = false;
		m_bInitMessage = false;
		m_Recipients.RemoveAll();
	}

	inline void	AddRecipient(int entindex)
	{
		// Already in list
		if (m_Recipients.Find(entindex) != m_Recipients.InvalidIndex())
			return;

		m_Recipients.AddToTail(entindex);
	}

	void RemoveAllRecipients(void)
	{
		m_Recipients.RemoveAll();
	}

	void RemoveRecipient(int entindex)
	{
		// Remove it if it's in the list
		m_Recipients.FindAndRemove(entindex);
	}

private:

	bool				m_bReliable;
	bool				m_bInitMessage;
	CUtlVector< int >	m_Recipients;

};


void EntityMessageBegin(CBaseEntity * entity, bool reliable = false);
void UserMessageBegin(IRecipientFilter& filter, const char *messagename);
void MessageEnd(void);

// bytewise
void MessageWriteByte(int iValue);
void MessageWriteChar(int iValue);
void MessageWriteShort(int iValue);
void MessageWriteWord(int iValue);
void MessageWriteLong(int iValue);
void MessageWriteFloat(float flValue);
void MessageWriteAngle(float flValue);
void MessageWriteCoord(float flValue);
void MessageWriteVec3Coord(const Vector& rgflValue);
void MessageWriteVec3Normal(const Vector& rgflValue);
void MessageWriteAngles(const QAngle& rgflValue);
void MessageWriteString(const char *sz);
void MessageWriteEntity(int iValue);
void MessageWriteEHandle(CBaseEntity *pEntity); //encoded as a long


// bitwise
void MessageWriteBool(bool bValue);
void MessageWriteUBitLong(unsigned int data, int numbits);
void MessageWriteSBitLong(int data, int numbits);
void MessageWriteBits(const void *pIn, int nBits);
// Bytewise
#define WRITE_BYTE		(MessageWriteByte)
#define WRITE_CHAR		(MessageWriteChar)
#define WRITE_SHORT		(MessageWriteShort)
#define WRITE_WORD		(MessageWriteWord)
#define WRITE_LONG		(MessageWriteLong)
#define WRITE_FLOAT		(MessageWriteFloat)
#define WRITE_ANGLE		(MessageWriteAngle)
#define WRITE_COORD		(MessageWriteCoord)
#define WRITE_VEC3COORD	(MessageWriteVec3Coord)
#define WRITE_VEC3NORMAL (MessageWriteVec3Normal)
#define WRITE_ANGLES	(MessageWriteAngles)
#define WRITE_STRING	(MessageWriteString)
#define WRITE_ENTITY	(MessageWriteEntity)
#define WRITE_EHANDLE	(MessageWriteEHandle)

// Bitwise
#define WRITE_BOOL		(MessageWriteBool)
#define WRITE_UBITLONG	(MessageWriteUBitLong)
#define WRITE_SBITLONG	(MessageWriteSBitLong)
#define WRITE_BITS	(MessageWriteBits)

#endif // !PLUGINUSERMESSAGES_HPP_