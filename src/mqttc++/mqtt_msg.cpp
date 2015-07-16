//
//  mqtt_msg.cpp
//  mqtt_c++
//
//  Created by davad.di on 7/13/15.
//  Copyright (c) 2015 davad.di. All rights reserved.
//

#include "mqttc++/mqtt_msg.hpp"

int CMqttFixedHeader::decode(uint8_t value)
{
    FixHeaderFlag header_flag;
    header_flag.all = value;
    
    m_retain_flag   = header_flag.bits.retain;
    m_qos           = header_flag.bits.qos;
    m_dup_flag      = header_flag.bits.dup;
    m_msg_type      = (MqttType)header_flag.bits.msg_type;
    
    return 0;
}

int CMqttFixedHeader::encode(uint8_t &fix_header)
{
    FixHeaderFlag header_flag;
    header_flag.all = 0;
    
    header_flag.bits.retain     = m_retain_flag;
    header_flag.bits.qos        = m_qos;
    header_flag.bits.dup        = m_dup_flag;
    header_flag.bits.msg_type   = (uint8_t)m_msg_type;
    
    fix_header = header_flag.all;
    
    return 1;
}

void CMqttFixedHeader::print()
{
    LOG_DEBUG("------------------------------------------------");
    LOG_DEBUG("Msgtype %d, dup_flag %d, retain_flag %d, qos %d",
              m_msg_type,m_dup_flag,m_retain_flag,m_qos);
}

int CMqttMsg::decode()
{
    // header
    uint8_t fixed_header = 0;
    if (m_mqtt_pkt.read_byte(fixed_header) < 0)
    {
        LOG_DEBUG("CMqttMsg::Get fix_header failed");
        return -1;
    }
    
    m_fixed_header.decode(fixed_header);
    
    if (m_mqtt_pkt.read_remain_length(m_remain_length_value, m_remain_length_bytes) < 0)
    {
        LOG_DEBUG("CMqttMsg::Get remain_lenght failed");
        return -1;
    }
    
    return 0;
}

int CMqttMsg::encode()
{
    uint8_t fix_header_byte;
    m_fixed_header.encode(fix_header_byte);
    
    m_mqtt_pkt.write_byte(fix_header_byte);
    
    return 0;
}


// -1 failed. 0 success
int CMqttConnect::decode()
{
    if (CMqttMsg::decode() < 0)
    {
        LOG_DEBUG("CMqttConnect:: CMqttMsg decode failed");
        return -1;
    }
    
    if (m_fixed_header.msg_type() != MqttType::CONNECT)
    {
        LOG_DEBUG("CMqttConnect::msg_type(%d) is not CONNECT", m_fixed_header.msg_type());
        return -1;
    }
    
    if (m_mqtt_pkt.read_string(m_str_proto_name) < 0)
    {
        LOG_DEBUG("CMqttConnect::Get protocol name failed");
        return -1;
    }
    
    if (m_mqtt_pkt.read_byte(m_proto_version) < 0)
    {
        LOG_DEBUG("CMqttConnect::Get version failed");
        return -1;
    }
    
    ConnectFlag flag;
    if (m_mqtt_pkt.read_byte(flag.all) < 0)
    {
        LOG_DEBUG("CMqttConnect::Get conn_flag failed");
        return -1;
    }
    
    if (m_mqtt_pkt.read_short(m_keep_alive) < 0)
    {
        LOG_DEBUG("CMqttConnect::Get keep alive failed");
        return -1;
    }
    
    if (m_mqtt_pkt.read_string(m_str_client_id) < 0)
    {
        LOG_DEBUG("CMqttConnect::Get client id failed");
        return -1;
    }
    
    if (flag.bits.will)
    {
        if (m_mqtt_pkt.read_string(m_str_will_topic)  < 0)
        {
            LOG_DEBUG("CMqttConnect::Get will topic failed");
            return -1;
        }
        
        if (m_mqtt_pkt.read_string(m_str_will_message) < 0)
        {
            LOG_DEBUG("CMqttConnect::Get will msg failed");
            return -1;
        }
    }
    
    if (flag.bits.username)
    {
        if (m_mqtt_pkt.read_string(m_str_user_name) < 0)
        {
            LOG_DEBUG("CMqttConnect::Get user name failed");
            return -1;
        }
    }
    
    if (flag.bits.password)
    {
        if (m_mqtt_pkt.read_string(m_str_password) < 0)
        {
            LOG_DEBUG("CMqttConnect::Get user passwd failed");
            return -1;
        }
    }
    
    return 0;
}

void CMqttConnect::print()
{
    m_fixed_header.print();
    LOG_DEBUG("Remain length %d, bytes %d", m_remain_length_value, m_remain_length_bytes);
    LOG_DEBUG("Portocol name %s, version %d", m_str_proto_name.c_str(), m_proto_version);
    
    LOG_DEBUG("Client id [%s], Clean session %d, Keep alive %d",
              m_str_client_id.c_str(), m_clean_session, m_keep_alive);
    
    if (m_has_will)
    {
        LOG_DEBUG("Will msg: qos [%d], retain [%d]", m_will_qos, m_has_will_retain);
        LOG_DEBUG("will topic [%s], msg [%s]", m_str_will_topic.c_str(), m_str_will_message.c_str());
    }
    
    if (m_has_user_name)
    {
        LOG_DEBUG("User name: [%s]", m_str_user_name.c_str());
    }
    
    if (m_has_password)
    {
        LOG_DEBUG("User passwd: [%s]", m_str_password.c_str());
    }
    
    LOG_DEBUG("------------------------------------------------");
}

int CMqttConnAck::encode()
{
    CMqttMsg::encode();
    
    m_remain_length_value = 2;
    m_mqtt_pkt.write_remain_length(m_remain_length_value, m_remain_length_bytes);
    
    m_reserved = 0;
    m_mqtt_pkt.write_byte(m_reserved);
    
    m_mqtt_pkt.write_byte((uint8_t)m_code);
    
    
    if (m_mqtt_pkt.length() != 4)
    {
        LOG_DEBUG("CMqttConnAck::encode length failed. should be 4");
        return -1;
    }
    

    return m_mqtt_pkt.length();
    
}

void CMqttConnAck::print()
{
    m_fixed_header.print();
    LOG_DEBUG("Code %d", m_code);
    LOG_DEBUG("------------------------------------------------");
}


int CMqttSubscribe::decode()
{
    if (CMqttMsg::decode() < 0)
    {
        LOG_DEBUG("CMqttSubscribe:: CMqttMsg decode failed");
        return -1;
    }
    
    if (m_fixed_header.msg_type() != MqttType::SUBSCRIBE)
    {
        LOG_DEBUG("CMqttSubscribe::msg_type(%d) is not SUBSCRIBE", m_fixed_header.msg_type());
        return -1;
    }
    
    if (m_mqtt_pkt.read_short(m_msg_id) < 0)
    {
        LOG_DEBUG("CMqttSubscribe::Read msg_id failed");
        return -1;
    }
    
    std::string str_topic_name;
    while(m_mqtt_pkt.read_string(str_topic_name) != FAILTURE)
    {
        uint8_t   topic_qos = 0;
        
        if (m_mqtt_pkt.read_byte(topic_qos) < 0)
        {
            LOG_DEBUG("Read topic qos failed.");
            break;
        }
        
        CTopic topic_node(str_topic_name, topic_qos);
        m_sub_topics.push_back(topic_node);
    }
    
    return 0;
}

void CMqttSubscribe::print()
{
    m_fixed_header.print();
    LOG_DEBUG("Remain length %d, bytes %d", m_remain_length_value, m_remain_length_bytes);
    LOG_DEBUG("Msg id [0x%x]", m_msg_id);
    
    int i = 1;
    for (auto it = m_sub_topics.begin(); it != m_sub_topics.end(); it++)
    {
        LOG_DEBUG("[%d] %s %d", i++, it->topic_name().c_str(), it->qos());
    }
    
    LOG_DEBUG("------------------------------------------------");
}

int CMqttSubAck::encode()
{
    CMqttMsg::encode();
    
    m_remain_length_value = m_sub_qos.size() + 2; // 2: msg_id
    m_mqtt_pkt.write_remain_length(m_remain_length_value, m_remain_length_bytes);
    
    m_mqtt_pkt.write_short(m_msg_id);
    
    for (auto it = m_sub_qos.begin(); it != m_sub_qos.end(); it++)
    {
        m_mqtt_pkt.write_byte(*it);
    }
    
    uint32_t pkt_len = m_remain_length_value + m_remain_length_bytes + 1; // 1 for fixed header
    uint32_t encode_pkt_len = m_mqtt_pkt.length();
    
    if ( encode_pkt_len != pkt_len) // 1 for fixed header
    {
        LOG_DEBUG("CMqttSubAck:: wrong encode lenght(%d), should be (%d)", encode_pkt_len, pkt_len);
        return -1;
    }
    
    return encode_pkt_len;
}

void CMqttSubAck::print()
{
    m_fixed_header.print();
    LOG_DEBUG("Remain length %d, bytes %d", m_remain_length_value, m_remain_length_bytes);
    LOG_DEBUG("Msg id [0x%x]", m_msg_id);
    
    int i = 1;
    for (auto it = m_sub_qos.begin(); it != m_sub_qos.end(); it++)
    {
        LOG_DEBUG("[%d] qos %d", i++, *it);
    }
    
    LOG_DEBUG("------------------------------------------------");
}

int CMqttUnsubscribe::decode()
{
    // header and remain lenght
    if (CMqttMsg::decode() < 0)
    {
        LOG_DEBUG("CMqttUnsubscribe:: CMqttMsg decode failed");
        return -1;
    }
    
    if (m_fixed_header.msg_type() != MqttType::UNSUBSCRIBE)
    {
        LOG_DEBUG("CMqttUnsubscribe::msg_type(%d) is not UNSUBSCRIBE", m_fixed_header.msg_type());
        return -1;
    }
    
    if (m_mqtt_pkt.read_short(m_msg_id) < 0)
    {
        LOG_DEBUG("CMqttUnsubscribe::Read msg_id failed");
        return -1;
    }
    
    std::string str_topic_name;
    while(m_mqtt_pkt.read_string(str_topic_name) != FAILTURE)
    {
        m_str_topics.push_back(str_topic_name);
    }
    
    return 0;
}

void CMqttUnsubscribe::print()
{
    m_fixed_header.print();
    LOG_DEBUG("Remain length %d, bytes %d", m_remain_length_value, m_remain_length_bytes);
    LOG_DEBUG("Msg id [0x%x]", m_msg_id);
    
    int i = 1;
    for (auto it = m_str_topics.begin(); it != m_str_topics.end(); it++)
    {
        LOG_DEBUG("[%d] topic %s", i++, it->c_str());
    }
    
    LOG_DEBUG("------------------------------------------------");
}

int CMqttUnsubAck::encode()
{
    CMqttMsg::encode();
    
    m_remain_length_value =  0x02; // 2: msg_id
    m_mqtt_pkt.write_remain_length(m_remain_length_value, m_remain_length_bytes);
    
    m_mqtt_pkt.write_short(m_msg_id);
    
    uint32_t pkt_len = 4; // include fixed header
    uint32_t encode_pkt_len = m_mqtt_pkt.length();
    
    if ( encode_pkt_len != pkt_len) // 1 for fixed header
    {
        LOG_DEBUG("CMqttUnsuback:: wrong encode lenght(%d), should be (%d)", encode_pkt_len, pkt_len);
        return -1;
    }
    
    return encode_pkt_len;
}


void CMqttUnsubAck::print()
{
    m_fixed_header.print();
    LOG_DEBUG("Remain length %d, bytes %d", m_remain_length_value, m_remain_length_bytes);
    LOG_DEBUG("Msg id [0x%x]", m_msg_id);
    LOG_DEBUG("------------------------------------------------");

}

int CMqttPublish::decode()
{
    // header and remain lenght
    if (CMqttMsg::decode() < 0)
    {
        LOG_DEBUG("CMqttPublish:: CMqttMsg decode failed");
        return -1;
    }
    
    // get topic name
    if (m_mqtt_pkt.read_string(m_str_topic_name) < 0)
    {
        LOG_DEBUG("CMqttPublish read topic name failed");
        return -1;
    }
    
    // get msg_id
    if (m_mqtt_pkt.read_short(m_msg_id) < 0)
    {
        LOG_DEBUG("CMqttPublish read msg_id failed");
        return -1;
    }
  
    
    // get payload
    if (m_mqtt_pkt.read_byte(m_payload, m_mqtt_pkt.left_size()) < 0)
    {
        LOG_DEBUG("CMqttPublish read payload failed");
        return -1;
    }
    
    return 0;
}

void CMqttPublish::print()
{
    m_fixed_header.print();
    LOG_DEBUG("Remain length %d, bytes %d", m_remain_length_value, m_remain_length_bytes);
    LOG_DEBUG("Msg id [0x%x]", m_msg_id);
    LOG_DEBUG("Topic name %s", m_str_topic_name.c_str());
    
    if (!m_payload.empty())
    {
        LOG_DEBUG("Payload len %d, first byte 0x%02x", (int)m_payload.size(), m_payload.front());
    }
    else
    {
        LOG_DEBUG("Payload empty");
    }
    
    LOG_DEBUG("------------------------------------------------");
}

int CMqttPingResp::encode()
{
    CMqttMsg::encode();
    
    m_remain_length_value =  0x0;
    m_mqtt_pkt.write_remain_length(m_remain_length_value, m_remain_length_bytes);
    
    
    uint32_t pkt_len = 2; // include fixed header
    uint32_t encode_pkt_len = m_mqtt_pkt.length();
    
    if ( encode_pkt_len != pkt_len)
    {
        LOG_DEBUG("CMqttPingResp:: wrong encode lenght(%d), should be (%d)", encode_pkt_len, pkt_len);
        return -1;
    }
    
    return encode_pkt_len;
}


