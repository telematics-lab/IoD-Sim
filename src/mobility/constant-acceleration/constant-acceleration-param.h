/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (C) 2018-2024 The IoD_Sim Authors.
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
#ifndef CONSTANT_ACCELERATION_PARAM_H
#define CONSTANT_ACCELERATION_PARAM_H

namespace ns3
{

/**
 * \ingroup mobility
 * \brief A class that holds the parameters for a constant acceleration.
 */
class ConstantAccelerationParam
{
  public:
    ConstantAccelerationParam(double acceleration, double maxSpeed);

    const double GetAcceleration() const;
    const double GetMaxSpeed() const;

  private:
    double m_acceleration;
    double m_maxSpeed;
};

} // namespace ns3

#endif /* CONSTANT_ACCELERATION_PARAM_H */
