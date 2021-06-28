/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018-2021 The IoD_Sim Authors.
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
#include "drone-server-application.h"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include "drone-communications.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DroneServerApplication");

NS_OBJECT_ENSURE_REGISTERED (DroneServerApplication);


TypeId
DroneServerApplication::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::DroneServerApplication")
    .SetParent<Application> ()
    .AddConstructor<DroneServerApplication> ()
    .AddAttribute ("Port", "Listening port.",
                   UintegerValue (80),
                   MakeUintegerAccessor (&DroneServerApplication::m_port),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Duration", "Duration of the application.",
                   DoubleValue (120.0),
                   MakeDoubleAccessor (&DroneServerApplication::m_duration),
                   MakeDoubleChecker<double> ())
  ;

  return tid;
}

DroneServerApplication::DroneServerApplication ()
{
  NS_LOG_FUNCTION (this);

  m_state = SERVER_CLOSED;
  m_sequenceNumber = 0;
}

DroneServerApplication::~DroneServerApplication ()
{
  NS_LOG_FUNCTION (this);

  m_state = SERVER_CLOSED;
}

void
DroneServerApplication::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_socket != NULL)
    m_socket->Close ();

  m_state = SERVER_CLOSED;

  Application::DoDispose ();
}

void
DroneServerApplication::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_state == SERVER_CLOSED)
    {
      if (m_socket == NULL)
        {
          Ptr<SocketFactory> socketFactory =
            GetNode ()->GetObject<SocketFactory> (UdpSocketFactory::GetTypeId ());

          m_socket = socketFactory->CreateSocket ();

          m_socket->SetAllowBroadcast (true);
          m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_port));
          m_socket->SetRecvCallback
            (MakeCallback (&DroneServerApplication::ReceivePacket, this));

          NS_LOG_INFO ("[Node " << GetNode ()->GetId () <<
                       "] new server socket (" << m_socket << ")");
        }

      m_state = SERVER_LISTEN;

      Simulator::Cancel (m_sendEvent);

      /* In case you want to broadcast data, uncomment this.
       *
       * for (double i = 1.0; i < m_duration; i += 30.0)
       *   Simulator::ScheduleWithContext (GetNode ()->GetId (),
       *                                   Seconds (i),
       *                                   &DroneServerApplication::SendUpdateBroadcast,
       *                                   this);
       */
    }
}

void
DroneServerApplication::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_socket != NULL && m_state == SERVER_LISTEN)
    {
      NS_LOG_LOGIC ("[Node " << GetNode ()->GetId () << "] Closing server socket");
      m_socket->Close ();
    }
}

void
DroneServerApplication::ReceivePacket (const Ptr<Socket> socket) const
{
  NS_LOG_FUNCTION (socket);

  Ptr<Packet> packet;
  Address senderAddr;
  Ipv4Address senderIpv4;

  while ((packet = socket->RecvFrom (senderAddr)))
    {
      if (InetSocketAddress::IsMatchingType (senderAddr))
        {
          senderIpv4 = InetSocketAddress::ConvertFrom (senderAddr)
                       .GetIpv4 ();

          NS_LOG_INFO ("[Node " << GetNode ()->GetId ()
                       << "] received " << packet->GetSize ()
                       << " bytes from " << senderIpv4);

          uint8_t *payload = (uint8_t *) calloc
            (packet->GetSize () + 1, sizeof (uint8_t));

          packet->CopyData (payload, packet->GetSize ());

          NS_LOG_INFO ("[Node " << GetNode ()->GetId ()
                        << "] packet contents: " << (char*) payload);

          rapidjson::Document d;
          d.Parse ((char *) payload);
          const char *commandStr = d["cmd"].GetString ();

          const PacketType command = PacketType (commandStr);
          switch (command)
            {
            case PacketType::HELLO:
              NS_LOG_INFO ("[Node " << GetNode ()->GetId ()
                          << "] HELLO packet!");
              m_sendEvent = Simulator::ScheduleNow (&DroneServerApplication::SendHelloAck,
                                                    this,
                                                    socket,
                                                    senderIpv4);
              break;
            case PacketType::UPDATE:
              NS_LOG_INFO ("[Node " << GetNode ()->GetId ()
                          << "] UPDATE packet!");
              m_sendEvent = Simulator::ScheduleNow (&DroneServerApplication::SendUpdateAck,
                                                    this,
                                                    socket,
                                                    senderIpv4);
              break;
            case PacketType::UPDATE_ACK:
              NS_LOG_INFO ("[Node " << GetNode ()->GetId ()
                          << "] UPDATE_ACK received!");
              break;
            default:
              NS_LOG_INFO ("[Node " << GetNode ()->GetId ()
                          << "] unknown packet received!");
            }

          free (payload);
        }
    }
}

void
DroneServerApplication::SendHelloAck (const Ptr<Socket> socket,
                           const Ipv4Address senderAddr) const
{
  NS_LOG_FUNCTION (socket << senderAddr);

  NS_LOG_INFO ("[Node " << GetNode ()->GetId ()
               << "] sending HELLO ACK back.");

  const std::string command = PacketType (PacketType::HELLO_ACK).ToString ();

  rapidjson::StringBuffer jsonBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer (jsonBuf);

  writer.StartObject ();
  writer.Key ("cmd");
  writer.String (command.c_str ());
  writer.Key ("sn");
  writer.Int (m_sequenceNumber++);
  writer.EndObject ();

  const char *json = jsonBuf.GetString ();
  const auto packet = Create<Packet>
    ((const uint8_t *) json, strlen (json) * sizeof (char));

  socket->SendTo (packet, 0, InetSocketAddress (senderAddr, m_port));
  m_txTrace (packet);
}

void
DroneServerApplication::SendUpdateAck (const Ptr<Socket> socket,
                           const Ipv4Address senderAddr) const
{
  NS_LOG_FUNCTION (socket << senderAddr);
  NS_LOG_INFO ("[Node " << GetNode ()->GetId ()
               << "] sending UPDATE ACK back.");

  const std::string command = PacketType (PacketType::UPDATE_ACK).ToString ();

  rapidjson::StringBuffer jsonBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer (jsonBuf);

  writer.StartObject ();
  writer.Key ("cmd");
  writer.String (command.c_str ());
  writer.Key ("sn");
  writer.Int (m_sequenceNumber++);
  writer.EndObject ();

  const char *json = jsonBuf.GetString ();
  const auto packet = Create<Packet>
    ((const uint8_t *) json, strlen (json) * sizeof (char));

  socket->SendTo (packet, 0, InetSocketAddress (senderAddr, m_port));
  m_txTrace (packet);
}

void
DroneServerApplication::SendUpdateBroadcast () const
{
  NS_LOG_FUNCTION_NOARGS ();

  const std::string command = PacketType (PacketType::UPDATE).ToString ();

  rapidjson::StringBuffer jsonBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer (jsonBuf);

  writer.StartObject ();
  writer.Key ("cmd");
  writer.String (command.c_str ());
  writer.Key ("sn");
  writer.Int (m_sequenceNumber);
  writer.EndObject ();

  const char *json = jsonBuf.GetString ();
  const auto packet = Create<Packet>
    ((const uint8_t *) json, strlen (json) * sizeof (char));

  NS_LOG_INFO ("[Node " << GetNode ()->GetId () << "] sending a Broadcast Update, NOW!");

  m_socket->SendTo (packet,
                    0,
                    InetSocketAddress (Ipv4Address::GetBroadcast (), m_port));

  m_txTrace (packet);
}

} // namespace ns3
