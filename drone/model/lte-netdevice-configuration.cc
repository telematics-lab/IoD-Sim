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
#include "lte-netdevice-configuration.h"

namespace ns3 {

class LteNetdeviceConfigurationPriv
{
public:
  static const LteRole ParseRole (const std::string rawRole)
  {
    if (rawRole.compare("UE") == 0)
      return LteRole::UE;
    else if (rawRole.compare("eNB") == 0)
      return LteRole::eNB;
    else
      NS_FATAL_ERROR ("Unsupported LTE Role: " << rawRole);
  }
};

LteNetdeviceConfiguration::LteNetdeviceConfiguration (const std::string type,
                                                      const std::string rawRole,
                                                      const std::vector<LteBearerConfiguration> bearers,
                                                      const uint32_t networkLayerId) :
  NetdeviceConfiguration {type, networkLayerId},
  m_role {LteNetdeviceConfigurationPriv::ParseRole (rawRole)},
  m_bearers {bearers}
{

}

LteNetdeviceConfiguration::~LteNetdeviceConfiguration ()
{

}

const LteRole
LteNetdeviceConfiguration::GetRole () const
{
  return m_role;
}

const std::vector<LteBearerConfiguration>
LteNetdeviceConfiguration::GetBearers () const
{
  return m_bearers;
}

const uint32_t
LteNetdeviceConfiguration::GetNetworkLayerId () const
{
  if (m_role == eNB)
    NS_FATAL_ERROR ("Cannot request the Network Layer ID for an eNB Configuration");

  return NetdeviceConfiguration::GetNetworkLayerId ();
}


} // namespace ns3
