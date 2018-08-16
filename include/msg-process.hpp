/*
    msg-process.hpp
*/

#ifndef _MSG_PROCESS_HPP_
#define _MSG_PROCESS_HPP_

namespace msg_process
{
    namespace json = rapidjson;

    template<typename T, size_t S, size_t BUF_SIZE = 1024>
    bool get_string_val(const json::GenericValue<T>& obj, const char(&key)[S], CString& sVal)
    {
        if(obj.HasMember(key))
        {
            const json::Value& val = obj[key];
            if(val.IsString())
            {
                if(val.GetStringLength() > 0)
                {
                    CA2WEX<BUF_SIZE> conv(val.GetString(), CP_UTF8);
                    sVal = conv;
                }
                else
                {
                    sVal.Empty();
                }

                return true;
            }
        }
        return false;
    }

    template<typename T>
    bool get_msg_cmd(const json::GenericDocument<T>& msg, CString& cmd)
    {
        return get_string_val(msg, "cmd", cmd);
    }

    template<typename T>
    bool get_msg_string_val(const json::GenericDocument<T>& msg, CString& val)
    {
        return get_string_val(msg, "val", val);
    }

    template<size_t INSTANCES, size_t BUFSIZE, typename S>
    void process_message(
        named_pipe_server<INSTANCES, BUFSIZE, S>& server,
        pipe_server_basics::INSTANCENO instanceNo,
        SM_SRV_HANDLER srv_handler,
        const pipe_server_basics::Buffer& buffer)
    {
        json::Document msg;
        json::ParseResult res = msg.Parse(reinterpret_cast<const char*>(buffer.GetData()), buffer.GetCount());
        if(res && msg.IsObject())
        {
            process_message(server, instanceNo, srv_handler, msg);
        }
    }

    template<size_t INSTANCES, size_t BUFSIZE, typename S, typename R>
    void process_message(
        named_pipe_server<INSTANCES, BUFSIZE, S>& server,
        pipe_server_basics::INSTANCENO instanceNo,
        SM_SRV_HANDLER srv_handler,
        json::GenericDocument<R>& msg)
    {
        bool f;
        CString sCmd;
        int nRes = SM_SRV_RPLY_NOREPLY;

        f = get_msg_cmd(msg, sCmd);
        if(!f) return;

        // SM_SRV_CMD_URL
        if(!sCmd.CompareNoCase(_T("url")))
        {
            CString sUrl;
            f = get_msg_string_val(msg, sUrl);
            if(!f || sUrl.IsEmpty()) return;

            nRes = srv_handler(SM_SRV_CMD_URL, const_cast<LPTSTR>((LPCTSTR)sUrl));
            send_message_to_sender(server, instanceNo, msg, nRes);
        }
    }

    template<size_t INSTANCES, size_t BUFSIZE, typename S, typename R>
    void send_message_to_sender(
        named_pipe_server<INSTANCES, BUFSIZE, S>& server,
        pipe_server_basics::INSTANCENO instanceNo,
        json::GenericDocument<R>& msg,
        int nReply)
    {
        if(nReply == SM_SRV_RPLY_NOREPLY)
        {
            return;
        }

        json::Document reply;
        reply.SetObject();
        auto& alloc = reply.GetAllocator();

        reply.AddMember("rply", nReply, alloc);
        reply.AddMember("cmd", msg["cmd"], alloc);
        reply.AddMember("val", msg["val"], alloc);

        send_message_to_sender(server, instanceNo, reply);
    }

    template<size_t INSTANCES, size_t BUFSIZE, typename S, typename R>
    void send_message_to_sender(
        named_pipe_server<INSTANCES, BUFSIZE, S>& server,
        pipe_server_basics::INSTANCENO instanceNo,
        const json::GenericDocument<R>& msg)
    {
        json::StringBuffer strm;
        json::Writer<json::StringBuffer> writer(strm);
        msg.Accept(writer);

        pipe_server_basics::Buffer buf_reply;
        size_t nBufferSize = strm.GetSize();
        buf_reply.SetCount(nBufferSize);
        Checked::memcpy_s(buf_reply.GetData(), nBufferSize, strm.GetString(), nBufferSize);

        server.SendMessage(instanceNo, buf_reply);
    }
};

#endif // _MSG_PROCESS_HPP_