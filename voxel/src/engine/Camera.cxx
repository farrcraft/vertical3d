/**
 * Voxel Engine
 *
 * (c) Joshua Farr <j.wgasa@gmail.com>
 *
 */

#include "Camera.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/norm.hpp>

Camera::Camera() :
	behavior_(CAMERA_BEHAVIOR_FLIGHT),
	fovx_(90.0f),
	near_(0.1f),
	far_(1000.0f),
	aspect_(1.0f),
	pitch_(0.0f),
	eye_(0.0f, 0.0f, 0.0f),
	xAxis_(1.0f, 0.0f, 0.0f),
    yAxis_(0.0f, 1.0f, 0.0f),
    zAxis_(0.0f, 0.0f, 1.0f),
    direction_(0.0f, 0.0f, -1.0f),
    acceleration_(0.0f, 0.0f, 0.0f),
    currentVelocity_(0.0f, 0.0f, 0.0f),
    velocity_(0.0f, 0.0f, 0.0f),
    view_(1.0f),
    projection_(1.0f)
{
}


const glm::vec3 &Camera::acceleration() const
{ 
	return acceleration_; 
}

Camera::CameraBehavior Camera::behavior() const
{ 
	return behavior_;
}

const glm::vec3 &Camera::currentVelocity() const
{ 
	return currentVelocity_; 
}

const glm::vec3 &Camera::position() const
{ 
	return eye_;
}

const glm::mat4 &Camera::projection() const
{ 
	return projection_;
}

const glm::vec3 &Camera::velocity() const
{ 
	return velocity_; 
}

const glm::vec3 &Camera::direction() const
{ 
	return direction_;
}

const glm::mat4 &Camera::view() const
{ 
	return view_;
}

const glm::vec3 &Camera::xAxis() const
{
	return xAxis_;
}

const glm::vec3 &Camera::yAxis() const
{ 
	return yAxis_; 
}

const glm::vec3 &Camera::zAxis() const
{ 
	return zAxis_; 
}

bool Camera::dirty() const
{
	return dirty_;
}

void Camera::dirty(bool status)
{
	dirty_ = status;
}

void Camera::lookAt(const glm::vec3 &target)
{
    lookAt(eye_, target, yAxis_);
}

void Camera::lookAt(const glm::vec3 &eye, const glm::vec3 &target, const glm::vec3 &up)
{
    eye_ = eye;

    zAxis_ = glm::normalize(eye - target);

    direction_ = -zAxis_;

	xAxis_ = glm::normalize(glm::cross(up, zAxis_));

	yAxis_ = glm::normalize(glm::cross(zAxis_, xAxis_));

    view_[0][0] = xAxis_.x;
    view_[1][0] = xAxis_.y;
    view_[2][0] = xAxis_.z;
    view_[3][0] = -glm::dot(xAxis_, eye);

    view_[0][1] = yAxis_.x;
    view_[1][1] = yAxis_.y;
    view_[2][1] = yAxis_.z;
    view_[3][1] = -glm::dot(yAxis_, eye);

    view_[0][2] = zAxis_.x;
    view_[1][2] = zAxis_.y;
    view_[2][2] = zAxis_.z;    
    view_[3][2] = -glm::dot(zAxis_, eye);

    // Extract the pitch angle from the view matrix.
	pitch_ = glm::degrees(-asinf(view_[1][2]));

	dirty_ = true;
}

void Camera::move(float dx, float dy, float dz)
{

	glm::vec3 eye = calculateMovement(glm::vec3(dx, dy, dz));
    position(eye);
}

glm::vec3 Camera::calculateMovement(const glm::vec3 & delta)
{
    // Moves the camera by dx world units to the left or right; dy
    // world units upwards or downwards; and dz world units forwards
    // or backwards.

	glm::vec3 eye = eye_;
    glm::vec3 forwards;

    if (behavior_ == CAMERA_BEHAVIOR_FIRST_PERSON)
    {
        // Calculate the forwards direction. Can't just use the camera's local
        // z axis as doing so will cause the camera to move more slowly as the
        // camera's view approaches 90 degrees straight up and down.

		forwards = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), xAxis_));
    }
    else
    {
        forwards = direction_;
    }
    
    eye += xAxis_ * delta.x;
    eye += glm::vec3(0.0f, 1.0f, 0.0f) * delta.y;
    eye += forwards * delta.z;

	return eye;
}

void Camera::move(const glm::vec3 &direction, const glm::vec3 &amount)
{
    // Moves the camera by the specified amount of world units in the specified
    // direction in world space.

    eye_.x += direction.x * amount.x;
    eye_.y += direction.y * amount.y;
    eye_.z += direction.z * amount.z;

    updateViewMatrix(false);
}

void Camera::perspective(float fovx, float aspect, float znear, float zfar)
{
    // Construct a projection matrix based on the horizontal field of view
    // 'fovx' rather than the more traditional vertical field of view 'fovy'.

	float e = 1.0f / tanf(glm::radians(fovx) / 2.0f);
    float aspectInv = 1.0f / aspect;
    float fovy = 2.0f * atanf(aspectInv / e);
    float xScale = 1.0f / tanf(0.5f * fovy);
    float yScale = xScale / aspectInv;

    projection_[0][0] = xScale;
    projection_[0][1] = 0.0f;
    projection_[0][2] = 0.0f;
    projection_[0][3] = 0.0f;

    projection_[1][0] = 0.0f;
    projection_[1][1] = yScale;
    projection_[1][2] = 0.0f;
    projection_[1][3] = 0.0f;

    projection_[2][0] = 0.0f;
    projection_[2][1] = 0.0f;
    projection_[2][2] = (zfar + znear) / (znear - zfar);
    projection_[2][3] = -1.0f;

    projection_[3][0] = 0.0f;
    projection_[3][1] = 0.0f;
    projection_[3][2] = (2.0f * zfar * znear) / (znear - zfar);
    projection_[3][3] = 0.0f;

    fovx_ = fovx;
    aspect_ = aspect;
    near_ = znear;
    far_ = zfar;
}

void Camera::rotate(float headingDegrees, float pitchDegrees, float rollDegrees)
{
    // Rotates the camera based on its current behavior.
    // Note that not all behaviors support rolling.

    switch (behavior_)
    {
	    case CAMERA_BEHAVIOR_FIRST_PERSON:
		    rotateFirstPerson(headingDegrees, pitchDegrees);
			break;
	    case CAMERA_BEHAVIOR_FLIGHT:
		    rotateFlight(headingDegrees, pitchDegrees, rollDegrees);
			break;
    }

    updateViewMatrix(true);
}

void Camera::rotateFlight(float headingDegrees, float pitchDegrees, float rollDegrees)
{
    glm::mat4 rotMtx;

    // Rotate camera's existing x and z axes about its existing y axis.
    if (headingDegrees != 0.0f)
    {
		rotMtx = glm::rotate(rotMtx, headingDegrees, yAxis_);
        xAxis_ = glm::vec3(glm::vec4(xAxis_, 0.0f) * rotMtx);
        zAxis_ = glm::vec3(glm::vec4(zAxis_, 0.0f) * rotMtx);
    }

    // Rotate camera's existing y and z axes about its existing x axis.
    if (pitchDegrees != 0.0f)
    {
        rotMtx = glm::rotate(rotMtx, pitchDegrees, xAxis_);
        yAxis_ = glm::vec3(glm::vec4(yAxis_, 0.0f) * rotMtx);
        zAxis_ = glm::vec3(glm::vec4(zAxis_, 0.0f) * rotMtx);
    }

    // Rotate camera's existing x and y axes about its existing z axis.
    if (rollDegrees != 0.0f)
    {
        rotMtx = glm::rotate(rotMtx, rollDegrees, zAxis_);
        xAxis_ = glm::vec3(glm::vec4(xAxis_, 0.0f) * rotMtx);
        yAxis_ = glm::vec3(glm::vec4(yAxis_, 0.0f) * rotMtx);
    }
}

void Camera::rotateFirstPerson(float headingDegrees, float pitchDegrees)
{
    pitch_ += pitchDegrees;

    if (pitch_ > 90.0f)
    {
        pitchDegrees = 90.0f - (pitch_ - pitchDegrees);
        pitch_ = 90.0f;
    }

    if (pitch_ < -90.0f)
    {
        pitchDegrees = -90.0f - (pitch_ - pitchDegrees);
        pitch_ = -90.0f;
    }

    glm::mat4 rotMtx;

    // Rotate camera's existing y and z axes about its existing x axis.
    if (pitchDegrees != 0.0f)
    {
        rotMtx = glm::rotate(rotMtx, pitchDegrees, xAxis_);
        yAxis_ = glm::vec3(glm::vec4(yAxis_, 0.0f) * rotMtx);
        zAxis_ = glm::vec3(glm::vec4(zAxis_, 0.0f) * rotMtx);
    }

    // Rotate camera's existing x and z axes about the world y axis.
    if (headingDegrees != 0.0f)
    {
        rotMtx = glm::rotate(rotMtx, headingDegrees, glm::vec3(0.0f, 1.0f, 0.0f));
        xAxis_ = glm::vec3(glm::vec4(xAxis_, 0.0f) * rotMtx);
        zAxis_ = glm::vec3(glm::vec4(zAxis_, 0.0f) * rotMtx);
    }

}

void Camera::acceleration(float x, float y, float z)
{
    acceleration_ = glm::vec3(x, y, z);
}

void Camera::acceleration(const glm::vec3 &acceleration)
{
    acceleration_ = acceleration;
}

void Camera::behavior(CameraBehavior newBehavior)
{
    if (behavior_ == CAMERA_BEHAVIOR_FLIGHT && newBehavior == CAMERA_BEHAVIOR_FIRST_PERSON)
    {
        // Moving from flight-simulator mode to first-person.
        // Need to ignore camera roll, but retain existing pitch and heading.

		lookAt(eye_, eye_ - zAxis_, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    behavior_ = newBehavior;
}

void Camera::currentVelocity(float x, float y, float z)
{
    currentVelocity_ = glm::vec3(x, y, z);
}

void Camera::currentVelocity(const glm::vec3 &currentVelocity)
{
    currentVelocity_ = currentVelocity;
}

void Camera::position(float x, float y, float z)
{
    eye_ = glm::vec3(x, y, z);
    updateViewMatrix(false);
}

void Camera::position(const glm::vec3 &position)
{
    eye_ = position;
    updateViewMatrix(false);
}

void Camera::velocity(float x, float y, float z)
{
    velocity_ = glm::vec3(x, y, z);
}

void Camera::velocity(const glm::vec3 &velocity)
{
    velocity_ = velocity;
}

glm::vec3 Camera::calculateDisplacement(const glm::vec3 &direction, float elapsedTimeSec)
{
	glm::vec3 displacement(0.0f);
	if (glm::length2(currentVelocity_) != 0.0f)
    {
        // Only move the camera if the velocity vector is not of zero length.
        // Doing this guards against the camera slowly creeping around due to
        // floating point rounding errors.

        displacement = (currentVelocity_ * elapsedTimeSec) +
            (0.5f * acceleration_ * elapsedTimeSec * elapsedTimeSec);

        // Floating point rounding errors will slowly accumulate and cause the
        // camera to move along each axis. To prevent any unintended movement
        // the displacement vector is clamped to zero for each direction that
        // the camera isn't moving in. Note that the updateVelocity() method
        // will slowly decelerate the camera's velocity back to a stationary
        // state when the camera is no longer moving along that direction. To
        // account for this the camera's current velocity is also checked.

		if (direction.x == 0.0f && glm::epsilonEqual(currentVelocity_.x, 0.0f, glm::epsilon<float>()))
		{
            displacement.x = 0.0f;
		}

		if (direction.y == 0.0f && glm::epsilonEqual(currentVelocity_.y, 0.0f, glm::epsilon<float>()))
		{
            displacement.y = 0.0f;
		}

		if (direction.z == 0.0f && glm::epsilonEqual(currentVelocity_.z, 0.0f, glm::epsilon<float>()))
		{
            displacement.z = 0.0f;
		}
	}
	return displacement;
}

void Camera::updatePosition(const glm::vec3 &direction, float elapsedTimeSec)
{
    // Moves the camera using Newton's second law of motion. Unit mass is
    // assumed here to somewhat simplify the calculations. The direction vector
    // is in the range [-1,1].

	glm::vec3 displacement = calculateDisplacement(direction, elapsedTimeSec);
	move(displacement.x, displacement.y, displacement.z);

    // Continuously update the camera's velocity vector even if the camera
    // hasn't moved during this call. When the camera is no longer being moved
    // the camera is decelerating back to its stationary state.

    updateVelocity(direction, elapsedTimeSec);
}

void Camera::updateVelocity(const glm::vec3 &direction, float elapsedTimeSec)
{
    // Updates the camera's velocity based on the supplied movement direction
    // and the elapsed time (since this method was last called). The movement
    // direction is in the range [-1,1].

    if (direction.x != 0.0f)
    {
        // Camera is moving along the x axis.
        // Linearly accelerate up to the camera's max speed.

        currentVelocity_.x += direction.x * acceleration_.x * elapsedTimeSec;

        if (currentVelocity_.x > velocity_.x)
            currentVelocity_.x = velocity_.x;
        else if (currentVelocity_.x < -velocity_.x)
            currentVelocity_.x = -velocity_.x;
    }
    else
    {
        // Camera is no longer moving along the x axis.
        // Linearly decelerate back to stationary state.

        if (currentVelocity_.x > 0.0f)
        {
            if ((currentVelocity_.x -= acceleration_.x * elapsedTimeSec) < 0.0f)
                currentVelocity_.x = 0.0f;
        }
        else
        {
            if ((currentVelocity_.x += acceleration_.x * elapsedTimeSec) > 0.0f)
                currentVelocity_.x = 0.0f;
        }
    }

    if (direction.y != 0.0f)
    {
        // Camera is moving along the y axis.
        // Linearly accelerate up to the camera's max speed.

        currentVelocity_.y += direction.y * acceleration_.y * elapsedTimeSec;

        if (currentVelocity_.y > velocity_.y)
            currentVelocity_.y = velocity_.y;
        else if (currentVelocity_.y < -velocity_.y)
            currentVelocity_.y = -velocity_.y;
    }
    else
    {
        // Camera is no longer moving along the y axis.
        // Linearly decelerate back to stationary state.

        if (currentVelocity_.y > 0.0f)
        {
            if ((currentVelocity_.y -= acceleration_.y * elapsedTimeSec) < 0.0f)
                currentVelocity_.y = 0.0f;
        }
        else
        {
            if ((currentVelocity_.y += acceleration_.y * elapsedTimeSec) > 0.0f)
                currentVelocity_.y = 0.0f;
        }
    }

    if (direction.z != 0.0f)
    {
        // Camera is moving along the z axis.
        // Linearly accelerate up to the camera's max speed.

        currentVelocity_.z += direction.z * acceleration_.z * elapsedTimeSec;

        if (currentVelocity_.z > velocity_.z)
            currentVelocity_.z = velocity_.z;
        else if (currentVelocity_.z < -velocity_.z)
            currentVelocity_.z = -velocity_.z;
    }
    else
    {
        // Camera is no longer moving along the z axis.
        // Linearly decelerate back to stationary state.

        if (currentVelocity_.z > 0.0f)
        {
            if ((currentVelocity_.z -= acceleration_.z * elapsedTimeSec) < 0.0f)
                currentVelocity_.z = 0.0f;
        }
        else
        {
            if ((currentVelocity_.z += acceleration_.z * elapsedTimeSec) > 0.0f)
                currentVelocity_.z = 0.0f;
        }
    }
}

void Camera::updateViewMatrix(bool orthogonalizeAxes)
{
    if (orthogonalizeAxes)
    {
        // Regenerate the camera's local axes to orthogonalize them.
        zAxis_ = glm::normalize(zAxis_);
		yAxis_ = glm::normalize(glm::cross(zAxis_, xAxis_));
		xAxis_ = glm::normalize(glm::cross(yAxis_, zAxis_));
        direction_ = -zAxis_;
    }

    // Reconstruct the view matrix.
    view_[0][0] = xAxis_.x;
    view_[1][0] = xAxis_.y;
    view_[2][0] = xAxis_.z;
    view_[3][0] = -glm::dot(xAxis_, eye_);

    view_[0][1] = yAxis_.x;
    view_[1][1] = yAxis_.y;
    view_[2][1] = yAxis_.z;
    view_[3][1] = -glm::dot(yAxis_, eye_);

    view_[0][2] = zAxis_.x;    
    view_[1][2] = zAxis_.y;
    view_[2][2] = zAxis_.z;   
    view_[3][2] = -glm::dot(zAxis_, eye_);

    view_[0][3] = 0.0f;
    view_[1][3] = 0.0f;
    view_[2][3] = 0.0f;
    view_[3][3] = 1.0f;

	dirty_ = true;
}