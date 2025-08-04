#ifndef	__DebugMessageHandler_H__
#define	__DebugMessageHandler_H__

#include <sstream>

class DebugMessageHandler : public std::stringstream
{
public:
	DebugMessageHandler(int level=9);
	virtual ~DebugMessageHandler();
	static void SetVerboseLevel( int level);

private:
	int mMyVerboseLevel;
};

#define		RUNTIME_ASSERT(exp) if (!(exp)) { throw runtime_error("Assertion Failed : " #exp " at " __FUNCTION__); }

#endif	//__DebugMessageHandler_H__