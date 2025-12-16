#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <Arduino.h>
#include "time.h"

#define MESSAGE_VERSION 1
#define MESSAGE_TYPE_UNKNOWN 0
#define MESSAGE_TYPE_VALUE 1
#define MESSAGE_TYPE_MAP 2
#define MESSAGE_OPTION_SIZE 1024
#define MESSAGE_BUFFER_SIZE 1024
#define MESSAGE_KEY_SIZE 64
#define MESSAGE_VALUE_SIZE 256

class Message
{
private:
	bool mPsramEnabled;
	char mOption[MESSAGE_OPTION_SIZE];
	char mBuffer[MESSAGE_BUFFER_SIZE];
	uint8_t *mData;
	uint32_t mSize;

public:
	uint8_t version;
	uint8_t type;

	Message();
	~Message();

	void enablePsram();

	uint32_t readInt32(uint8_t *src);
	void writeInt32(uint8_t *dst, uint32_t value);

	bool getOption(const char *name, char *buffer, uint32_t bufferSize);
	// uint32_t getOptionLength();
	size_t getOptionLength();

	void setOption(const char *name, const char *value);
	void setOption(const char *format, ...);

	void setLastModified();
	void setLastModified(const char *value);
	void setDataType(const char *value);

	uint8_t *getData();
	bool getString(char *buffer, uint32_t bufferSize);
	bool getJSON(const char *key, char *buffer, uint32_t bufferSize);
	bool getJSON(const char *key, int *buffer);
	bool getJSON(const char *key, int *buffer, uint32_t bufferSize);
	bool getJSON(const char *key, float *buffer);

	void setData(uint8_t *data, uint32_t dataSize);
	bool setValue(const char *format, ...);

	uint32_t getSize();
	void setSize(uint32_t size);
	void setSize(uint32_t size, bool reallocate);

	bool fromPayload(uint8_t *payload, uint32_t payloadSize);
	uint32_t toPayload(uint8_t *buffer, uint32_t bufferSize);

	void reset();
};

#endif