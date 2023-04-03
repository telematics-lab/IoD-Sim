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
 *
 * Authors: Giovanni Grieco <giovanni.grieco@poliba.it>, Giovanni Iacovelli
 * <giovanni.iacovelli@poliba.it>
 */
#ifndef REPORT_LOCATION_H
#define REPORT_LOCATION_H
#include <ns3/nstime.h>
#include <ns3/vector.h>

#include <libxml/xmlwriter.h>

namespace ns3
{

class ReportLocation
{
  public:
    /**
     * Initialize a report location with a given position, instant, and region of interest
     */
    ReportLocation(Vector position, Time instant, int roi);
    /**
     * Initialize a report location with a given position and instant
     */
    ReportLocation(Vector position, Time instant);
    /**
     * Default constructor for commodity purposes
     */
    ReportLocation();
    /**
     * Default destructor
     */
    ~ReportLocation();
    /**
     * \return the position vector
     */
    Vector GetPosition();
    /**
     * Write Zsp report data to a XML file with a given handler
     *
     * \param handle the XML handler to write data on
     */
    void Write(xmlTextWriterPtr handle);

  private:
    Vector m_position; /// the position descibing the location
    Time m_instant;    /// the time at which this position is assumed
    int m_roi;
};

} // namespace ns3

#endif /* REPORT_LOCATION_H */
