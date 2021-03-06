//
//  acceptor.cpp
//
//
//  Created by davad.di on 7/15/15.
//
//

#include "mqtt_server/mqtt_connection.hpp"
#include "mqtt_server/acceptor.hpp"

namespace reactor
{
    int Acceptor::open(const CSockAddress &address)
    {
        LOG_TRACE_METHOD(__func__);
        
        if (m_sock_acceptor.open(address, 1024) == -1)
        {
            LOG_ERROR("Acceptor open failed.");
            return -1;
        }
        
        this->set_handle(m_sock_acceptor.get_handle());
        LOG_INFO("Acceptor open succeed. handler %d", m_sock_acceptor.get_handle());
        
        return CEventHandler::open(); // register handler, with EV_READ event
    }
    
    int Acceptor::handle_input(socket_t UNUSED(sock_id))
    {
        LOG_TRACE_METHOD(__func__);
        
        CEventHandler *event_handler = new CMqttConnection(this->m_reactor_ptr);
        
        int socket_id = this->m_sock_acceptor.accept(event_handler);
        
        LOG_INFO("New client arrived, sock_id [%d]", socket_id);
        
        event_handler->open();
        
        return 0;
    }
}

