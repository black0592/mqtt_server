#ifndef	 _mqtt_server_topic_mgr__
#define  _mqtt_server_topic_mgr__

#include <unordered_set>
#include "common/mbuf.hpp"
#include "common/singleton.hpp"
//#include "mqtt_server/mqtt_context.hpp"
#include "mqtt_server/mqtt_connection.hpp"


namespace reactor
{
    typedef std::unordered_set<CMqttClientContext_ptr> CONTEXT_SET;

    class CTopicNode
    {
	public:
	    CTopicNode(){}

	    void update_retain_msg(CMqttPubMessage &retain_msg)
	    {
		m_retain_msg = retain_msg;
	    }

	    CMqttPubMessage &retain_msg()
	    {
		return m_retain_msg;
	    }

	    CONTEXT_SET & client_context()
	    {
		return m_subcriber_clients;
	    }

	    int add_client(CMqttClientContext_ptr &cli_context)
	    {
		if (m_subcriber_clients.find(cli_context) == m_subcriber_clients.end())
		{
		    m_subcriber_clients.insert(cli_context);
		    return 0;
		}

		LOG_DEBUG("Add client context has already existed.");
		return -1;
	    }

	    int del_client(CMqttClientContext_ptr &cli_context)
	    {
		auto it = m_subcriber_clients.find(cli_context);

		if (it == m_subcriber_clients.end())
		{
		    LOG_DEBUG("Del cli_context is not exist");
		    return -1;
		}

		m_subcriber_clients.erase(it);

		return 0;
	    }

	    void print()
	    {
		uint32_t i = 1;
		for (auto it = m_subcriber_clients.begin(); it != m_subcriber_clients.end(); it++, i++)
		{
		    LOG_DEBUG("[%d] %s", i, (*it)->client_id().c_str());
		}
	    }

	private:
	    CMqttPubMessage	    m_retain_msg;
	    CONTEXT_SET	    m_subcriber_clients;
    };

    typedef std::shared_ptr<CTopicNode> CTopicNode_ptr;

    class CSubscriberMgr
    {
	public:
	    CSubscriberMgr(){}
	    ~CSubscriberMgr(){}

	    int add_client_context(std::string str_topic_name, CMqttClientContext_ptr cli_context)
	    {
		auto it = m_topic_mgr.find(str_topic_name);

		if (it == m_topic_mgr.end()) // topic is not exists
		{
		    CTopicNode_ptr topic_node = make_shared<CTopicNode>();

		    topic_node->add_client(cli_context);

		    m_topic_mgr[str_topic_name] = topic_node;
		}
		else
		{
		    it->second->add_client(cli_context);
		}

		return 0;
	    }

	    int del_client_context(std::string &str_topic_name, CMqttClientContext_ptr & cli_context)
	    {
		auto it = m_topic_mgr.find(str_topic_name);

		if (it != m_topic_mgr.end())
		{
		    it->second->del_client(cli_context);

		    return 0;
		}

		LOG_DEBUG("del client context not find client_id[%s] in topic [%s]",
			cli_context->client_id().c_str(), str_topic_name.c_str());

		return -1;
	    }

	    int find_client_context(std::string &str_topic_name, CONTEXT_SET &clients_set)
	    {
		auto it = m_topic_mgr.find(str_topic_name);

		if (it != m_topic_mgr.end()) 
		{
		    clients_set =  it->second->client_context();
		    return 0;
		}

		LOG_DEBUG("No client on topic [%s]", str_topic_name.c_str());

		return -1;
	    }

	    int publish(std::string &str_topic_name, CMbuf_ptr &mbuf)
	    {
		auto it = m_topic_mgr.find(str_topic_name);
		if (it == m_topic_mgr.end())
		{
		    LOG_DEBUG("No subscriber find here");
		    return -1;
		}

		CONTEXT_SET &client_context_set = it->second->client_context();
		for (auto it = client_context_set.begin(); it != client_context_set.end(); it++)        
		{                          
		    // it mean client_context object
		    auto mqtt_conn = (*it)->mqtt_connection();                                          
		    if (mqtt_conn != nullptr)                                                           
		    {                                                                                   
			mqtt_conn->put(mbuf);                                                       
		    }                                                                                   
		}

		return 0;
	    }

	    void print()
	    {
		int i = 1;

		LOG_DEBUG("\n");
		LOG_DEBUG("------- CSubscriberMgr -------------");
		for (auto it = m_topic_mgr.begin(); it != m_topic_mgr.end(); it++, i++)
		{
		    LOG_DEBUG("[%d] %s", i, it->first.c_str());
		    LOG_DEBUG("------------------------------------");
		    it->second->print();
		    LOG_DEBUG("------------------------------------");  
		}
		LOG_DEBUG("-------- CSubscriberMgr end ----------\n");  

	    }


	protected:
	    std::unordered_map<std::string,CTopicNode_ptr> m_topic_mgr;
    };

    typedef CSingleton<CSubscriberMgr> SubscriberMgr;

#define SUB_MGR   SubscriberMgr::instance()

} // end of namespace

#endif
