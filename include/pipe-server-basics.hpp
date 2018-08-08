/*
    pipe-server-basics.hpp
*/

#ifndef _PIPE_SERVER_BASICS_HPP_
#define _PIPE_SERVER_BASICS_HPP_

#include "named-pipe.hpp"

class pipe_server_basics
{
protected:

    static const DWORD PIPE_CONNECT_TIMEOUT = 5000;

    enum INSTANCE_STATE
    {
        INSTANCE_STATE_CONNECTING,
        INSTANCE_STATE_READING,
        INSTANCE_STATE_WRITING
    };

	static const size_t MAX_INPUT_MESSAGE_SIZE = 1024 * 1024 * 1024;

public:

    typedef ULONG64 INSTANCENO;
    typedef CAtlArray<BYTE> Buffer;

    class INotify
    {
    public:
        virtual void OnConnect(INSTANCENO) = 0;
        virtual void OnMessage(INSTANCENO, const Buffer&) = 0;
        virtual void OnDisconnect(INSTANCENO) = 0;
    };
};

#endif // _PIPE_SERVER_BASICS_HPP_
