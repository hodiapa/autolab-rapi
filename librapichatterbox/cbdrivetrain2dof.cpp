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
#include "cbdrivetrain2dof.h"
#include "utilities.h"
#include "rapierror.h"
#include "cbodometry.h"

namespace Rapi
{

//-----------------------------------------------------------------------------
CCBDrivetrain2dof::CCBDrivetrain2dof ( CCBDriver* driver, std::string devName )
    : ADrivetrain2dof ( devName )
{
  mCBDriver = driver;
  mOIMode = CB_MODE_SAFE;

  // maximal change in velocities
  mMaxTurnRateDelta = D2R ( 5 );
  mMaxVelocityDelta = 0.1;

  mUpperVelocityLimit = CVelocity2d (  0.5, 0.0,  D2R ( 30.0 ) );
  mLowerVelocityLimit = CVelocity2d ( -0.5, 0.0, -D2R ( 30.0 ) );

  mOdometry = new CCBOdometry ( mCBDriver, devName+":Odometry" );

  setEnabled( true );
}
//-----------------------------------------------------------------------------
CCBDrivetrain2dof::~CCBDrivetrain2dof()
{
  if ( mOdometry )
    delete mOdometry;
}
//-----------------------------------------------------------------------------
int CCBDrivetrain2dof::init()
{
  return 1; // success
}
//-----------------------------------------------------------------------------
void CCBDrivetrain2dof::setEnabled ( bool enable )
{
  mFgEnabled = enable;
}
//-----------------------------------------------------------------------------
void CCBDrivetrain2dof::setSpeedCmd ( const float velocity,
                                      const float turnrate )
{
  mVelocityCmd = CVelocity2d ( velocity, 0.0, turnrate );
}
//-----------------------------------------------------------------------------
void CCBDrivetrain2dof::setSpeedCmd ( CVelocity2d velocity )
{
  mVelocityCmd = velocity;
}
//-----------------------------------------------------------------------------
void CCBDrivetrain2dof::updateData()
{
  CPose2d pose;
  static CPose2d lastPose;
  static int count = 0;
  static int noProgressCount = 0;


  if ( mFgEnabled == true ) {
    printf("start ");
    mVelocityCmd.print();

    // read current velocities from chatterbox
    mVelocityMeas.mVX = ( double ) ( mCBDriver->mCreateSensorPackage.velocity ) /1000.0;
    mVelocityMeas.mVY = 0.0;
    if ( mCBDriver->mCreateSensorPackage.radius >= 0x7FFF )  // 0x7FFF or 0x8000
      mVelocityMeas.mYawDot = 0;
    else if ( mCBDriver->mCreateSensorPackage.radius == 0xFFFF )
      mVelocityMeas.mYawDot = D2R ( -40.0 );  // TODO: find real maximum turnrate
    else if ( mCBDriver->mCreateSensorPackage.radius == 0x0001 )
      mVelocityMeas.mYawDot = D2R ( 40.0 );   // TODO: find real maximum turnrate
    else if ( mCBDriver->mCreateSensorPackage.radius != 0 )
      mVelocityMeas.mYawDot = mCBDriver->mCreateSensorPackage.velocity /
                              mCBDriver->mCreateSensorPackage.radius;

    // limit acceleration
    if ( fabs ( mVelocityMeas.mVX - mVelocityCmd.mVX ) > mMaxVelocityDelta ) {
      mVelocityCmd.mVX += SIGN ( mVelocityCmd.mVX - mVelocityMeas.mVX ) *
                          mMaxVelocityDelta;
    }

    if ( fabs ( mVelocityMeas.mYawDot - mVelocityCmd.mYawDot ) > mMaxTurnRateDelta ) {
      mVelocityCmd.mYawDot += SIGN ( mVelocityCmd.mYawDot - mVelocityMeas.mYawDot ) *
                              mMaxTurnRateDelta;
    }
    printf("before limit ");
    mVelocityCmd.print();
    // limit speeds
    applyVelocityLimits();
    printf("after limit ");
    mVelocityCmd.print();
    // set OpenInterface Mode
    if ( mCBDriver->mCreateSensorPackage.oiMode != mOIMode )
      mCBDriver->setOIMode ( mOIMode );

    // set speeds
    if ( mCBDriver->setSpeed ( mVelocityCmd ) == 0 ) {
      ERROR2 ( "Failed to set speed command v=%f w=%f", mVelocityCmd.mVX,
               mVelocityCmd.mYawDot );
    }

    // check if we are stuck
    pose = mOdometry->getPose();

    if ( count > 10 ) {
      count = 0;
      if ( fabs ( pose.distance ( lastPose ) ) < 0.01 )
        noProgressCount++;
      else
        noProgressCount = 0;

      if ( noProgressCount > 50 )
        mFgStalled = true;
      else
        mFgStalled = false;

      lastPose = pose;
    }
    count ++;
  } // enabled
}
//-----------------------------------------------------------------------------
void CCBDrivetrain2dof::print()
{
  printf ( "Drive: v=%01.3f m/s, w=%03.3f deg/s ",
           mVelocityCmd.mVX,
           R2D ( mVelocityCmd.mYawDot ) );
  mOdometry->print();

  switch ( mCBDriver->mCreateSensorPackage.oiMode ) {
    case CB_MODE_OFF:
      printf ( " mode: OFF " );
      break;
    case CB_MODE_PASSIVE:
      printf ( " mode: PASSIVE " );
      break;
    case CB_MODE_SAFE:
      printf ( " mode: SAFE " );
      break;
    case CB_MODE_FULL:
      printf ( " mode: FULL " );
      break;
  } // switch
  printf ( "\n" );
}
//-----------------------------------------------------------------------------
void CCBDrivetrain2dof::setDefaultOIMode ( tOIMode mode )
{
  mOIMode = mode;
}
//-----------------------------------------------------------------------------
tOIMode CCBDrivetrain2dof::getOIMode()
{
  return mCBDriver->getOIMode();
}
//-----------------------------------------------------------------------------
int CCBDrivetrain2dof::activateDemo ( tDemo demo )
{
  if ( mCBDriver->activateDemo ( demo ) == 1 ) {
    if ( demo == CB_DEMO_STOP )
      mOIMode = CB_MODE_SAFE;
    else
      mOIMode = CB_MODE_PASSIVE;

    return 1; // success
  }
  return 0; // failure
}
//-----------------------------------------------------------------------------

} // namespace
