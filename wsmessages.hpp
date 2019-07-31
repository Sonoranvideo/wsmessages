#ifndef __SVS_WSMESSAGES_H__
#define __SVS_WSMESSAGES_H__

#ifndef WIN32
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif //WIN32

#include <stdint.h>
#include <vector>
#include <string.h>

#ifdef LWS_SEND_BUFFER_PRE_PADDING

#define WSMSGS_LWSBUF_START LWS_SEND_BUFFER_PRE_PADDING
#define WSMSGS_LWSBUF_END LWS_SEND_BUFFER_POST_PADDING
#define WSMSGS_LWSBUF_TOTAL (WSMSGS_LWSBUF_START + WSMSGS_LWSBUF_END)
#else
#define WSMSGS_LWSBUF_START 0
#define WSMSGS_LWSBUF_END 0
#define WSMSGS_LWSBUF_TOTAL 0

#endif //LWS_SEND_BUFFER_PRE_PADDING

class WSMessage
{
private:
	static constexpr size_t PreBodySize = WSMSGS_LWSBUF_START + sizeof(uint32_t);
	static constexpr size_t PostBodySize = WSMSGS_LWSBUF_END;
	static constexpr size_t BodyOffset = PreBodySize;
	static constexpr size_t SizeBinOffset = WSMSGS_LWSBUF_TOTAL;
	
	std::vector<uint8_t> Buffer;
	size_t SeekOffset;

	void EncodeSize(const size_t BodyLength)
	{
		const uint32_t Value = htonl(BodyLength);
		
		memcpy(this->Buffer.data() + SizeBinOffset, &Value, sizeof Value);
	}
	
	void Resize(const size_t BodyLength)
	{
		this->Buffer.resize(PreBodySize + BodyLength);
		
		memset(this->Buffer.data(), 0, PreBodySize);
		
		this->EncodeSize(BodyLength);
		
		memset((this->Buffer.data() + this->Buffer.size()) - PostBodySize, 0, PostBodySize);
	}
	
public:

	inline WSMessage(const void *Body, const size_t BodyLength)
	: Buffer(), SeekOffset()
	{
		this->Resize(BodyLength);
		
		memcpy(this->Buffer.data() + BodyOffset, Body, BodyLength);
	}
	
	
	uint8_t *GetBody(void) { return this->Buffer.data() + BodyOffset; }
	const uint8_t *GetBody(void) const { return this->Buffer.data() + BodyOffset; }
	
	uint8_t *GetSeekedBody(void) { return this->Buffer.data() + BodyOffset + this->SeekOffset; }
	const uint8_t *GetSeekedBody(void) const { return this->Buffer.data() + BodyOffset + this->SeekOffset; }
	
	uint8_t *GetSeekedRawData(void) { return this->Buffer.data() + this->SeekOffset; }
	const uint8_t *GetSeekedRawData(void) const { return this->Buffer.data() + this->SeekOffset; }
	
	uint32_t GetBodySize(void) const { return this->DecodeMsgSize(this->Buffer.data() + SizeBinOffset); }
	uint32_t GetRemainingSize(void) const { return this->DecodeMsgSize(this->Buffer.data() + SizeBinOffset) - this->SeekOffset; }
	uint32_t GetBufferSize(void) const { return this->Buffer.size(); }
	
	size_t GetPosition(void) const
	{
		return this->SeekOffset;
	}
	
	bool Seek(const size_t Offset = 0)
	{
		if (Offset >= this->GetBodySize()) return false;
		
		SeekOffset = Offset;
		
		return true;
	}
	
	void RawSeekForward(const size_t Increment)
	{
		this->RawSeek(this->SeekOffset + Increment);
	}
	
	bool SeekForward(const size_t Increment)
	{
		return this->Seek(this->SeekOffset + Increment);
	}
	
	void RawSeek(const size_t Offset = 0)
	{
		SeekOffset = Offset;
	}
	
	static uint32_t DecodeMsgSize(const void *Data)
	{
		uint32_t Value = 0;
		
		memcpy(&Value, Data, sizeof Value);
		
		return ntohl(Value);
	}
	
	class Fragment
	{
	private:
		static constexpr size_t DataOffset = sizeof(uint32_t);
		
		std::vector<uint8_t> Buffer;
		size_t CompletedSize;
	public:
		Fragment(const void *Data, const size_t DataLength)
		: CompletedSize(WSMessage::DecodeMsgSize(Data))
		{
			this->Buffer.resize(DataLength);
			memcpy(this->Buffer.data(), Data, DataLength);
		}
		
		void Append(const void *Data, const size_t DataLength)
		{
			const size_t OldSize = this->Buffer.size();
			this->Buffer.resize(DataLength + OldSize);
			
			memcpy(this->Buffer.data() + OldSize, Data, DataLength);
		}

		bool IsComplete(void) const
		{
			return this->Buffer.size() == this->CompletedSize + DataOffset;
		}
		
		WSMessage *Graduate(void) const
		{
			return new WSMessage(this->Buffer.data() + DataOffset, this->CompletedSize);
		}
		
		friend class WSMessage;
	};
};
#endif //__SVS_WSMESSAGES_H__
