#include "mqtt_server/xml_config.hpp"

int CXMLConfig::read_node_text(tinyxml2::XMLElement *elment, const char *name, std::string &str_value)
{
    str_value.clear();
    tinyxml2::XMLElement *node = elment->FirstChildElement(name);
    if (node == nullptr)
    {
        LOG_ERROR("Cannot find node for %s", name);
        return -1;
    }
    else
    {
        str_value =  node->GetText();
        if (str_value.empty())
        {
            LOG_ERROR("Get node %s value is empty", name);
            return -1;
        }
    }
    
    return 0;
}


int CXMLConfig::open()
{
    if (m_file_name.empty())
    {
        fprintf(stderr, "Config file name is empty");
        return -1;
    }
    
    tinyxml2::XMLDocument doc;
    doc.LoadFile(m_file_name.c_str());
    
    if (doc.ErrorID())
    {
        fprintf(stderr, "Open file name [%s], failed err %d,  %s\n",m_file_name.c_str(), doc.ErrorID(),doc.GetErrorStr2());
        return -1;
    }
    
    
    // global
    // <global>
    //      <thread_num>1</thread_num>
    //      <log4cplus_conf>log4cplus_debug.properties</log4cplus_conf>
    // </global>
    
    tinyxml2::XMLElement *global = doc.FirstChildElement("global");
    if ( global == nullptr )
    {
        fprintf(stderr, "Cann't find xml node for global");
        return -1;
    }
    
    std::string tmp;
    tinyxml2::XMLElement *node = global->FirstChildElement("log4cplus_conf");
    if (node == nullptr)
    {
        return -1;
    }
    
    m_log_conf_name = node->GetText();
    
    // call  PropertyConfigurator::doConfigure(file_name) to setting log
    CLoggerMgr logger(m_log_conf_name.c_str());
    
    //ERROR_RETURN(this->read_node_text(global, "log4cplus_conf", m_log_conf_name), -1);
    ERROR_RETURN(this->read_node_text(global, "thread_num", tmp), -1);
    m_thread_number = atoi(tmp.c_str()); // for later
    
    // mqtt_server
    //<mqtt_server>
    //      <ip>0.0.0.0</ip>
    //      <port>5060</port>
    //      <db_file_name>dup.db</db_file_name>
    //      <db_flush_interval>60</db_flush_interval>
    //      <max_idle_time>300</max_idle_time>
    //      <data_dir>./</data_dir>
    //</mqtt_server>
    
    tinyxml2::XMLElement *mqtt_server = doc.FirstChildElement("mqtt_server");
    if ( mqtt_server == nullptr )
    {
        LOG_ERROR("Cann't find xml node for mqtt_server");
        return -1;
    }
    
    ERROR_RETURN(this->read_node_text(mqtt_server, "ip", m_server_listen_ip), -1);
    ERROR_RETURN(this->read_node_text(mqtt_server, "port", tmp), -1);
    m_server_listen_port = (uint16_t)atoi(tmp.c_str());
    
    ERROR_RETURN(this->read_node_text(mqtt_server, "db_file_name", m_db_file_name), -1);
    
    ERROR_RETURN(this->read_node_text(mqtt_server, "db_flush_interval", tmp), -1);
    m_flush_interval = atoi(tmp.c_str());
    if (m_flush_interval <= 0)
    {
        m_flush_interval = 5*60; // 5 min
    }
    
    ERROR_RETURN(this->read_node_text(mqtt_server, "max_idle_timeout", tmp), -1);
    m_max_idle_timeout = atoi(tmp.c_str());
    
    if (m_max_idle_timeout <= 0)
    {
        m_max_idle_timeout = 300; // 5min
    }
    
    ERROR_RETURN(this->read_node_text(mqtt_server, "data_dir", m_data_dir), -1);
    
    // <mqtt_bridge>1</mqtt_bridge>
    tinyxml2::XMLElement *bridge = doc.FirstChildElement("mqtt_bridge");
    tmp = bridge->GetText();
    if (tmp.empty())
    {
        LOG_ERROR("Bridge mode must be setted");
        return -1;
    }
    m_mqtt_bridge = atoi(tmp.c_str());
    
    //<parent_mqtt_server>
    //      <ip>127.0.0.1</ip>
    //      <port>5050</port>
    //      <user_name>diwh</user_name>
    //      <topic_name>topic_name</topic_name>
    //      <keep_alive>60</keep_alive>
    //</parent_mqtt_server>
    
    if (m_mqtt_bridge == 0 || m_mqtt_bridge == 3)
    {
        tinyxml2::XMLElement *parent_mqtt_server = doc.FirstChildElement("parent_mqtt_server");
        if ( mqtt_server == nullptr )
        {
            LOG_ERROR("Cann't find xml node for parent_mqtt_server");
            return -1;
        }
        
        ERROR_RETURN(this->read_node_text(parent_mqtt_server, "ip", m_parent_server_ip), -1);
        ERROR_RETURN(this->read_node_text(parent_mqtt_server, "port", tmp), -1);
        
        m_parent_server_port = (uint16_t)atoi(tmp.c_str());
        
        ERROR_RETURN(this->read_node_text(parent_mqtt_server, "user_name", m_parent_user_name), -1);
        ERROR_RETURN(this->read_node_text(parent_mqtt_server, "topic_name", m_parent_topic_name), -1);
        
        ERROR_RETURN(this->read_node_text(parent_mqtt_server, "keep_alive", tmp), -1);
        
        m_parent_keep_alive = (uint16_t)atoi(tmp.c_str());
        
        if (m_parent_keep_alive <= 0)
        {
            m_parent_keep_alive = 60;
        }
    }
    
    if (m_mqtt_bridge >= 1)
    {
        // <http_server>  <!-- bridge -->
        //	<ip>0.0.0.0</ip>
        //	<port>8080</port>
        // </http_server>
        tinyxml2::XMLElement *http_server = doc.FirstChildElement("http_server");
        ERROR_RETURN(this->read_node_text(http_server, "ip", m_http_server_ip), -1);
        ERROR_RETURN(this->read_node_text(http_server, "port", tmp), -1);
        m_http_server_port = (uint16_t)atoi(tmp.c_str());
    }
    
    return 0;
}


void CXMLConfig::print()
{
    LOG_INFO("\t ------- Global --------------------");
    LOG_INFO("\t  Setting file name [%s]",  m_file_name.c_str());
    LOG_INFO("\t  Log conf file name [%s]", m_log_conf_name.c_str());
    LOG_INFO("\t  Thread number [%d]\n", m_thread_number);
    
    LOG_INFO("\t ------- MQTT Server ----------------");
    LOG_INFO("\t  Server listen at [%s:%d]", m_server_listen_ip.c_str(), m_server_listen_port);
    LOG_INFO("\t  DB AND TIMOUT Check Interval [%d]", m_flush_interval);
    LOG_INFO("\t  DB file name [%s]", m_db_file_name.c_str());
    LOG_INFO("\t  Max client timeout [%d]", m_max_idle_timeout);
    LOG_INFO("\t  Data dir %s\n", m_data_dir.c_str());
    
    
    if (m_mqtt_bridge == 0 || m_mqtt_bridge == 3 )
    {
        
        LOG_INFO("\t  ----- MODE [Mqtt_server]  -------------");
        
        LOG_INFO("\t ------- Parent MQTT Server ----------");
        LOG_INFO("\t  Parent Server Addr [%s:%d]", m_parent_server_ip.c_str(), m_parent_server_port);
        LOG_INFO("\t  UserName [%s], TopicName [%s], KeepAlive [%d]\n",
                 m_parent_user_name.c_str(), m_parent_topic_name.c_str(), m_parent_keep_alive);
    }
    
    if (m_mqtt_bridge >= 1)
    {
        
        LOG_INFO("\t  ----- MODE [Bridge]  -------------");
        
        LOG_INFO("\t ------ Http Server Info  -----------");
        LOG_INFO("\t Server start at %s:%d", m_http_server_ip.c_str(), m_http_server_port);
    }
    
    LOG_INFO("\t ------------------------------------\n");
    
}

