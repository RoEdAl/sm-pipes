/*
    pipe-client-basics.hpp
*/

#ifndef _PIPE_CLIENT_BASICS_HPP_
#define _PIPE_CLIENT_BASICS_HPP_

class pipe_client_basics
{
protected:

	static const size_t MAX_INPUT_MESSAGE_SIZE = 1024 * 1024 * 1024;
	static const DWORD CONNECT_TIMEOUT = 1000;
	static const DWORD WRITE_RETRY_TIMEOUT = 50;

public:

	typedef CAtlArray<BYTE> Buffer;

    class INotify
    {
    public:
        virtual void OnConnect() = 0;
        virtual void OnMessage(const Buffer&) = 0;
        virtual void OnDisconnect() = 0;
    };
};

#endif // _PIPE_CLIENT_BASICS_HPP_
