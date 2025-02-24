/**
*    File: trajectory.h
*    Created by Hawkeye 10/2011
*
*    Generate joint and cartesian trajectories.
*    Internal datastructures track trajectory state, and update DOFs as needed upon calling.
*/

#include "trajectory.h"
#include "log.h"
#include "utils.h"
#include "defines.h"
#include <ros/ros.h>

#include <memory>
#include <boost/thread/mutex.hpp>

extern unsigned long int gTime;

// Store trajectory parameters
struct _trajectory
{
    ros::Time startTime;
    float end_pos;
    float magnitude;
    float period;
    float startPos;
    float startVel;
};
struct _trajectory trajectory[MAX_MECH*MAX_DOF_PER_MECH];


/**
*  start_velocity_trajectory()
*    initialize trajectory parameters
*/
int start_trajectory(struct DOF* _joint, float _endPos, float _period)
{
    trajectory[_joint->type].startTime = trajectory[_joint->type].startTime.now();
    trajectory[_joint->type].startPos = _joint->jpos;
    trajectory[_joint->type].startVel = _joint->jvel;
    _joint->jpos_d = _joint->jpos;
    _joint->jvel_d = _joint->jvel;

    trajectory[_joint->type].magnitude = _endPos - _joint->jpos;
    trajectory[_joint->type].period = _period;
//    log_msg("starting trajectory on joint %d to magnitude: %0.3f (%0.3f - %0.3f), period:%0.3f",
//        _joint->type,
//        trajectory[_joint->type].magnitude,
//        _endPos, _joint->jpos,
//        trajectory[_joint->type].period);
    return 0;
}
/**
*  start_velocity_trajectory()
*    initialize trajectory parameters
*/
int start_trajectory_mag(struct DOF* _joint, float _mag, float _period)
{
    trajectory[_joint->type].startTime = trajectory[_joint->type].startTime.now();
    trajectory[_joint->type].startPos = _joint->jpos;
    trajectory[_joint->type].startVel = _joint->jvel;
    _joint->jpos_d = _joint->jpos;
    _joint->jvel_d = _joint->jvel;

    trajectory[_joint->type].magnitude = _mag;
    trajectory[_joint->type].period = _period;
    return 0;
}

/**
*   stop_velocity_trajectory()
*      Set jvel zero
*      Zero torque
*/
int stop_trajectory(struct DOF* _joint)
{
    trajectory[_joint->type].startTime = trajectory[_joint->type].startTime.now();
    trajectory[_joint->type].startPos = _joint->jpos;
    trajectory[_joint->type].startVel = 0;
    _joint->jpos_d = _joint->jpos;
    _joint->jvel_d = 0;
    _joint->tau_d = 0;
    _joint->current_cmd = 0;

    return 0;
}

/**
*  update_sinusoid_trajectory()
*        find next trajectory waypoint
*        Sinusoid trajcetory
*/
int update_sinusoid_velocity_trajectory(struct DOF* _joint)
{
    const float maxspeed = 15 DEG2RAD;
    const float f_period = 2000;         // 2 sec

    ros::Duration t = ros::Time::now() - trajectory[_joint->type].startTime;

   if (_joint->type      == SHOULDER_GOLD)
        _joint->jvel_d = -1 * maxspeed * sin( 2*M_PI * (1/f_period) * t.toSec());

//    else if (_joint->type == ELBOW_GOLD)
//        _joint->jvel_d =  maxspeed * sin( 2*M_PI * (1/f_period) * t);
//
//    else if (_joint->type == Z_INS_GOLD)
//        _joint->jvel_d =  0.2 * sin( 2*M_PI * (1/f_period) * t);

    else
        _joint->jvel_d = 0;

    return 0;
}

/**
*
*  update_linear_sinusoid_trajectory()
*     find next trajectory waypoint.
*     Sinusoid ramp up and linear velocity after peak.
*/
int update_linear_sinusoid_velocity_trajectory(struct DOF* _joint)
{
    const float maxspeed[8] = {-4 DEG2RAD, 4 DEG2RAD, 0.02, 15 DEG2RAD};
    const float f_period = 2;         // 2 sec

    ros::Duration t = ros::Time::now() - trajectory[_joint->type].startTime;

    // Sinusoid portion complete.  Return without changing velocity.
    if (t.toSec() >= f_period/2)
        return 1;

    if (_joint->type      == SHOULDER_GOLD)
        _joint->jvel_d = maxspeed[0] * (1-cos( 2*M_PI * (1/f_period) * t.toSec()));

    else if (_joint->type == ELBOW_GOLD)
        _joint->jvel_d = maxspeed[1] * (1-cos( 2*M_PI * (1/f_period) * t.toSec()));

    else if (_joint->type == Z_INS_GOLD)
        _joint->jvel_d = maxspeed[2] * (1-cos( 2*M_PI * (1/f_period) * t.toSec()));

    else
        _joint->jvel_d = 0;

    return 0;
}

/**
*  update_sinusoid_position_trajectory()
*     find next trajectory waypoint.
*     Sinusoidal position trajectory
*/
int update_sinusoid_position_trajectory(struct DOF* _joint)
{
    struct _trajectory* traj = &(trajectory[_joint->type]);
    float f_magnitude = traj->magnitude;
    float f_period    = traj->period;

    ros::Duration t = ros::Time::now() - traj->startTime;

    // Rising sinusoid
    if ( t.toSec() < f_period/4 )
        _joint->jpos_d = -f_magnitude * 0.5 * (1-cos( 4 * M_PI * t.toSec() / f_period )) + traj->startPos;
    else
        _joint->jpos_d = -f_magnitude * sin( 2*M_PI * t.toSec() / f_period) + traj->startPos;

    return 0;
}

/**
*  update_sinusoid_position_trajectory()
*     find next trajectory waypoint.
*     Sinusoidal position trajectory
*/
int update_linear_sinusoid_position_trajectory(struct DOF* _joint)
{
//    const float f_magnitude[8] = {-10 DEG2RAD, 10 DEG2RAD, 0.01, 0, 60 DEG2RAD, 60 DEG2RAD, 60 DEG2RAD, 60 DEG2RAD};
//    const float f_period[8] = {7000, 3200, 7000, 0000, 5000, 5000, 5000, 5000};
    struct _trajectory* traj = &(trajectory[_joint->type]);

    ros::Duration t = ros::Time::now() - traj->startTime;

    if ( t.toSec() < traj->period/2 )
//        _joint->jpos_d += ONE_MS * f_magnitude[index] * (1-cos( 2*M_PI * (1/f_period[index]) * t.toSec()));
        _joint->jpos_d += ONE_MS * traj->magnitude * (1-cos( 2*M_PI * (1/traj->period) * t.toSec()));
    else
        _joint->jpos_d += ONE_MS * traj->magnitude;

    return 0;
}


/**
*  update_sinusoid_position_trajectory()
*     find next trajectory waypoint.
*     Sinusoidal position trajectory
*/
int update_position_trajectory(struct DOF* _joint)
{
    struct _trajectory* traj = &(trajectory[_joint->type]);
    float magnitude = traj->magnitude;
    float period  = traj->period;

    ros::Duration t = ros::Time::now()- traj->startTime;

    if ( t.toSec() < period ){
        _joint->jpos_d = 0.5*magnitude * (1-cos( 2*M_PI * (1/(2*period)) * t.toSec())) + traj->startPos;
        return 1;
    }

    return 0;
}

boost::mutex trajMutex;

std::auto_ptr<param_pass_trajectory> traj;

bool setTrajectory(const param_pass_trajectory& new_traj) {
	trajMutex.lock();
	if (!traj.get()) {
		traj.reset(new param_pass_trajectory(new_traj));
	} else {
		*traj = new_traj;
	}
	//printf("set traj with %u steps, %f %f %f\n",traj->pts.size(),traj->begin_time,traj->total_duration,traj->begin_time + traj->total_duration);
	trajMutex.unlock();
	return true;
}
bool clearTrajectory() {
	trajMutex.lock();
	traj.reset();
	trajMutex.unlock();
	return true;
}
bool hasTrajectory() {
	bool hasTraj;
	trajMutex.lock();
	hasTraj = traj.get() != 0;
	trajMutex.unlock();
	return hasTraj;
}
bool getTrajectory(param_pass_trajectory& traj_out) {
	bool gotTraj = false;
	trajMutex.lock();
	if (traj.get()) {
		traj_out = *traj;
		gotTraj = true;
	}
	trajMutex.unlock();
	return gotTraj;
}
TrajectoryStatus getCurrentTrajectoryParams(t_controlmode& controller,param_pass& param) {
	TrajectoryStatus ret = TrajectoryStatus::NO_TRAJECTORY;
	ros::Time now = ros::Time::now();
	trajMutex.lock();
	if (traj.get()) {
		if (traj->total_duration != 0 && now.toSec() > traj->begin_time + traj->total_duration) {
			//printf("Clearing traj %f %f %f %f\n",traj->begin_time,traj->total_duration,traj->begin_time + traj->total_duration,now.toSec());
			traj.reset();
			ret = TrajectoryStatus::ENDED;
		} else {
			controller = traj->control_mode;
			int ind_to_use = -1;
			for (int i=0;i<(int)traj->pts.size();i++) {
				if (traj->begin_time + traj->pts[i].time_from_start < now.toSec()) {
					ind_to_use = i;
				} else {
					break;
				}
			}
			if (ind_to_use != -1) {
				//log_msg_throttle(0.25,"Time left: %f",traj->begin_time + traj->pts.back().time_from_start - now.toSec());
				param = traj->pts[ind_to_use].param;
				ret = TrajectoryStatus::OK;
			} else {
				//log_msg_throttle(0.25,"before start, first: %f %f %f %f",traj->begin_time, traj->pts[0].time_from_start,traj->begin_time + traj->pts[0].time_from_start,now.toSec());
				ret = TrajectoryStatus::BEFORE_START;
			}
		}
	}
	trajMutex.unlock();
	return ret;
}
