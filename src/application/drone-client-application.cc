/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (C) 2018-2023 The IoD_Sim Authors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "drone-client-application.h"

#include "drone-communications.h"

#include <ns3/core-module.h>
#include <ns3/drone-list.h>
#include <ns3/drone-peripheral.h>
#include <ns3/internet-module.h>
#include <ns3/network-module.h>
#include <ns3/storage-peripheral.h>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DroneClientApplication");
NS_OBJECT_ENSURE_REGISTERED(DroneClientApplication);

TypeId
DroneClientApplication::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::DroneClientApplication")
            .SetParent<Application>()
            .AddConstructor<DroneClientApplication>()
            .AddAttribute("DestinationIpv4Address",
                          "IPv4 Address of the destination device",
                          Ipv4AddressValue(Ipv4Address::GetBroadcast()),
                          MakeIpv4AddressAccessor(&DroneClientApplication::m_destAddr),
                          MakeIpv4AddressChecker())
            .AddAttribute("Port",
                          "Destination application port.",
                          UintegerValue(80),
                          MakeUintegerAccessor(&DroneClientApplication::m_destPort),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("TransmissionInterval",
                          "Interval between the transmission of packets, in seconds.",
                          DoubleValue(1.0),
                          MakeDoubleAccessor(&DroneClientApplication::m_interval),
                          MakeDoubleChecker<double>())
            .AddAttribute("InitialHandshake",
                          "Flag for initial HELLO handshake between client and server.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&DroneClientApplication::m_initialHandshakeEnable),
                          MakeBooleanChecker())
            .AddAttribute("FreeData",
                          "Free data if the StoragePeripheral is available.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&DroneClientApplication::m_storage),
                          MakeBooleanChecker())
            .AddTraceSource("Tx",
                            "A new packet is created and is sent",
                            MakeTraceSourceAccessor(&DroneClientApplication::m_txTrace),
                            "ns3::Packet::TracedCallback");

    return tid;
}

DroneClientApplication::DroneClientApplication()
{
    NS_LOG_FUNCTION(this);

    m_state = CLOSED;
    m_sequenceNumber = 0;
}

DroneClientApplication::~DroneClientApplication()
{
    NS_LOG_FUNCTION(this);

    m_state = CLOSED;
}

void
DroneClientApplication::DoDispose()
{
    NS_LOG_FUNCTION(this);

    if (m_socket != NULL)
        m_socket->Close();

    m_state = CLOSED;

    Application::DoDispose();
}

void
DroneClientApplication::StartApplication()
{
    NS_LOG_FUNCTION(this);

    if (m_socket == NULL)
    {
        Ptr<SocketFactory> socketFactory =
            GetNode()->GetObject<SocketFactory>(UdpSocketFactory::GetTypeId());
        m_socket = socketFactory->CreateSocket();

        m_socket->SetAllowBroadcast(true);
        m_socket->SetRecvCallback(MakeCallback(&DroneClientApplication::ReceivePacket, this));

        NS_LOG_INFO("[Node " << GetNode()->GetId() << "] new client socket (" << m_socket << ")");

        /* set CourseChange callback using ns-3 XPath addressing system */
        const uint32_t nodeId = GetNode()->GetId();
        std::stringstream xPathCallback;
        xPathCallback << "/NodeList/" << nodeId << "/$ns3::MobilityModel/CourseChange";
        Config::Connect(xPathCallback.str(),
                        MakeCallback(&DroneClientApplication::CourseChange, this));
    }

    Simulator::Cancel(m_sendEvent);

    if (m_initialHandshakeEnable)
    {
        m_sendEvent = Simulator::ScheduleNow(&DroneClientApplication::SendPacket,
                                             this,
                                             NEW,
                                             m_socket,
                                             m_destAddr);
    }
    else
    {
        m_state = CONNECTED;

        for (double i = Simulator::Now().GetSeconds(); i < m_stopTime.GetSeconds(); i += m_interval)
        {
            Simulator::ScheduleWithContext(GetNode()->GetId(),
                                           Seconds(i),
                                           &DroneClientApplication::SendPacket,
                                           this,
                                           NEW,
                                           m_socket,
                                           m_destAddr);
        }
    }
}

void
DroneClientApplication::StopApplication()
{
    NS_LOG_FUNCTION(this);

    Simulator::Cancel(m_sendEvent);

    if (m_socket != NULL)
    {
        NS_LOG_LOGIC("[Node " << GetNode()->GetId() << "] Closing client socket");
        m_socket->Close();
    }
}

void
DroneClientApplication::SendPacket(const Intent i,
                                   const Ptr<Socket> socket,
                                   const Ipv4Address targetAddress) const
{
    NS_LOG_FUNCTION(this << ToString(i) << socket << targetAddress);

    if (m_socket != NULL)
    {
        const char* command;
        const auto nodeId = GetNode()->GetId();

        if (m_state == CLOSED && i == NEW)
        {
            command = PacketType(PacketType::HELLO).ToString();
            m_state = HELLO_SENT;
        }
        else if (m_state == CONNECTED)
        {
            switch (i)
            {
            case NEW:
                command = PacketType(PacketType::UPDATE).ToString();
                break;
            case ACK:
                command = PacketType(PacketType::UPDATE_ACK).ToString();
                break;
            }
        }
        else
        {
            NS_LOG_WARN("[Node " << nodeId << "] SendPacket"
                                 << " reached an untained state.");
            return;
        }

        rapidjson::StringBuffer jsonBuf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(jsonBuf);

        // Try to get node info about current position and velocity
        const auto mobilityModel = GetNode()->GetObject<MobilityModel>();
        const auto pos = mobilityModel->GetPosition();
        const auto vel = mobilityModel->GetVelocity();

        writer.StartObject();
        writer.Key("id");
        writer.Int(nodeId);
        writer.Key("sn"); // Sequence Number
        writer.Int(m_sequenceNumber++);
        writer.Key("cmd");
        writer.String(command);
        writer.Key("gps");
        writer.StartObject();
        writer.Key("lat");
        writer.Double(pos.x);
        writer.Key("lon");
        writer.Double(pos.y);
        writer.Key("alt");
        writer.Double(pos.z);
        writer.Key("vel");
        writer.StartArray();
        writer.Double(vel.x);
        writer.Double(vel.y);
        writer.Double(vel.z);
        writer.EndArray();
        writer.EndObject();
        writer.EndObject();

        const char* json = jsonBuf.GetString();
        Ptr<Packet> packet = Create<Packet>((const uint8_t*)json, strlen(json) * sizeof(char));

        socket->SendTo(packet, 0, InetSocketAddress(targetAddress, m_destPort));
        if (GetNode()->GetInstanceTypeId().GetName() == "ns3::Drone" &&
            DroneList::GetDrone(nodeId)->getPeripherals()->thereIsStorage() && m_storage)
        {
            Ptr<StoragePeripheral> storage = StaticCast<StoragePeripheral, DronePeripheral>(
                DroneList::GetDrone(nodeId)->getPeripherals()->Get(0));
            if (storage->Free(strlen(json) * sizeof(char), StoragePeripheral::byte))
                NS_LOG_INFO("[Node " << GetNode()->GetId() << "] Freed "
                                     << strlen(json) * sizeof(char) << " bytes ");
        }
        m_txTrace(packet);

        NS_LOG_INFO("[Node " << GetNode()->GetId() << "] sending packet " << json << " to "
                             << targetAddress << ":" << m_destPort);
    }
    else
    {
        NS_LOG_ERROR("[Node " << GetNode()->GetId()
                              << "] called SendPacket but socket is not initialized yet!");
    }
}

void
DroneClientApplication::ReceivePacket(const Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    Ptr<Packet> packet;
    Address senderAddr;

    while ((packet = socket->RecvFrom(senderAddr)))
    {
        if (InetSocketAddress::IsMatchingType(senderAddr))
        {
            const auto senderIpv4 = InetSocketAddress::ConvertFrom(senderAddr).GetIpv4();

            NS_LOG_INFO("[Node " << GetNode()->GetId() << "] client received " << packet->GetSize()
                                 << " bytes from " << senderIpv4);

            uint8_t* payload = (uint8_t*)calloc(packet->GetSize() + 1, sizeof(uint8_t));
            packet->CopyData(payload, packet->GetSize());

            NS_LOG_INFO("[Node " << GetNode()->GetId() << "] packet contents: " << (char*)payload);

            rapidjson::Document d;
            d.Parse((char*)payload);
            const char* command = d["cmd"].GetString();

            if (PacketType(command) == PacketType::HELLO_ACK && m_state == HELLO_SENT)
            {
                m_destAddr = senderIpv4;

                NS_LOG_INFO("[Node " << GetNode()->GetId() << "] received HELLO_ACK with IP "
                                     << m_destAddr);

                m_state = CONNECTED;

                // It's safe to use Simulator::Now() here because we execute this code during the
                // simulation. If we use Application Start Time attribute, we risk to insert events
                // in the past, which is pure garbage.
                for (double i = Simulator::Now().GetSeconds(); i < m_stopTime.GetSeconds();
                     i += m_interval)
                {
                    Simulator::ScheduleWithContext(GetNode()->GetId(),
                                                   Seconds(i),
                                                   &DroneClientApplication::SendPacket,
                                                   this,
                                                   NEW,
                                                   m_socket,
                                                   m_destAddr);
                }
            }
            else if (PacketType(command) == PacketType::UPDATE_ACK && m_state == CONNECTED)
            {
                NS_LOG_INFO("[Node " << GetNode()->GetId() << "] UPDATE_ACK received!");
            }
            else if (PacketType(command) == PacketType::UPDATE && m_state == CONNECTED)
            {
                NS_LOG_INFO("[Node " << GetNode()->GetId() << "] UPDATE received!");

                m_sendEvent = Simulator::ScheduleNow(&DroneClientApplication::SendPacket,
                                                     this,
                                                     ACK,
                                                     socket,
                                                     senderIpv4);
            }

            free(payload);
        }
    }
}

void
DroneClientApplication::CourseChange(const std::string context,
                                     const Ptr<const MobilityModel> mobility) const
{
    // NS_LOG_FUNCTION (context << mobility);

    const Vector position = mobility->GetPosition();
    const Vector velocity = mobility->GetVelocity();

    NS_LOG_INFO(Simulator::Now() << " [Node " << GetNode()->GetId() << "]"
                                 << "; Pos: (" << position.x << ":" << position.y << ":"
                                 << position.z << "); Vel: (" << velocity.x << ":" << velocity.y
                                 << ":" << velocity.z << ")");
}

} // namespace ns3
