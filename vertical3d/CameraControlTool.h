#ifndef INCLUDED_V3D_CAMERACONTROLTOOL
#define INCLUDED_V3D_CAMERACONTROLTOOL

#include "Tool.h"
#include <vertical3d/3dtypes/ArcBall.h>
#include <vertical3d/3dtypes/Vector.h>
#include <vertical3d/input/MouseEventListener.h>
#include <vertical3d/input/MouseDevice.h>
#include "ViewPort.h"

#include <boost/shared_ptr.hpp>

/**
 * The CameraControlTool provides interactive manipulation of a Camera through a ViewPort.
 */
class CameraControlTool : 
	public v3D::Tool,
	public v3D::MouseEventListener
{
	public:
		CameraControlTool(const boost::shared_ptr<v3D::MouseDevice> & mouse, const boost::shared_ptr<v3D::ViewPort> & view);

		typedef enum CameraMode
		{
			CAMERA_MODE_ZOOM,
			CAMERA_MODE_TRUCK,
			CAMERA_MODE_PAN,
			CAMERA_MODE_NONE
		} CameraMode;

		// tool overrides
		void activate(const std::string & name);
		void deactivate(const std::string & name);

		// mouse event listener overrides
		void motion(unsigned int x, unsigned int y);
		void buttonPressed(unsigned int button);
		void buttonReleased(unsigned int button);

		// these each happen when there is a motion event and the primary mouse button is pressed
		void zoom(const v3D::Vector2ui & position);
		void truck(const v3D::Vector2ui & position);
		void pan(const v3D::Vector2ui & position);

	private:
		v3D::Vector2ui last_;
		CameraMode mode_;
		boost::shared_ptr<v3D::ViewPort> view_;
		v3D::ArcBall arcball_;
		boost::shared_ptr<v3D::MouseDevice> mouse_;
};


#endif // INCLUDED_V3D_CAMERACONTROLTOOL
