/***************************************************************************
 * Project: RAPI                                                           *
 * Author:  Jens Wawerla (jwawerla@sfu.ca)                                 *
 * $Id: $
 ***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 **************************************************************************/

#include "stageodometry.h"
#include "utilities.h"

namespace Rapi
{

//-----------------------------------------------------------------------------
CStageOdometry::CStageOdometry ( Stg::ModelPosition* stgModel,
                                 std::string devName )
    : COdometry ( devName )
{
  assert(stgModel);
  mStgPosition = stgModel;
}
//-----------------------------------------------------------------------------
CStageOdometry::~CStageOdometry()
{
}
//-----------------------------------------------------------------------------
void CStageOdometry::updateData( const double dt)
{
  Stg::Pose pose;

  pose = mStgPosition->GetPose();
  // mPose.mX = pose.x;
  // mPose.mY = pose.y;
  // mPose.mYaw = pose.a;

  // Coordinate system tranformation
  double angle = mCoordinateSystemOffset.mYaw;
  mPose.mX =  pose.x * cos( angle ) + pose.y * sin( angle )
              + mCoordinateSystemOffset.mX;
  mPose.mY = -pose.x * sin( angle ) + pose.y * cos( angle )
              + mCoordinateSystemOffset.mY;
  mPose.mYaw = normalizeAngle( pose.a - angle );

  mTimeStamp = mStgPosition->GetWorld()->SimTimeNow() / 1e6;
}
//-----------------------------------------------------------------------------

} // namespace
