#include "RenderMan.h"
//#include "Polygon.h"
#include "Renderer.h"

#include <stdarg.h>

#include <string>
#include <string.h>

using namespace v3d::moya;


Renderer _renderer;


RtToken RI_FRAMEBUFFER, 
		RI_FILE;
RtToken RI_RGB, 
		RI_RGBA, 
		RI_RGBZ, 
		RI_RGBAZ, 
		RI_A, 
		RI_Z, 
		RI_AZ;
const RtToken RI_PERSPECTIVE = const_cast<char*>("perspective");
const RtToken RI_ORTHOGRAPHIC = const_cast<char*>("orthographic");
RtToken RI_HIDDEN, 
		RI_PAINT;
RtToken RI_CONSTANT, 
		RI_SMOOTH;
RtToken RI_FLATNESS;
const RtToken RI_FOV = const_cast<char*>("fov");
RtToken RI_AMBIENTLIGHT, 
		RI_POINTLIGHT, 
		RI_DISTANTLIGHT, 
		RI_SPOTLIGHT;
RtToken RI_INTENSITY, 
		RI_LIGHTCOLOR, 
		RI_FROM, 
		RI_TO, 
		RI_CONEANGLE,
		RI_CONEDELTAANGLE, 
		RI_BEAMDISTRIBUTION;
RtToken RI_MATTE, 
		RI_METAL, 
		RI_SHINYMETAL, 
		RI_PLASTIC, 
		RI_PAINTEDPLASTIC;
RtToken RI_KA, 
		RI_KD, 
		RI_KS, 
		RI_ROUGHNESS, 
		RI_KR, 
		RI_TEXTURENAME, 
		RI_SPECULARCOLOR;
RtToken RI_DEPTHCUE, 
		RI_FOG, 
		RI_BUMPY;
RtToken RI_MINDISTANCE, 
		RI_MAXDISTANCE, 
		RI_BACKGROUND, 
		RI_DISTANCE,
		RI_AMPLITUDE;
RtToken RI_RASTER, 
		RI_SCREEN, 
		RI_CAMERA, 
		RI_WORLD, 
		RI_OBJECT;
RtToken RI_INSIDE, 
		RI_OUTSIDE, 
		RI_LH, 
		RI_RH;
const RtToken RI_P = const_cast<char*>("P");
const RtToken RI_PZ = const_cast<char*>("Pz");
const RtToken RI_PW = const_cast<char*>("Pw");
const RtToken RI_N = const_cast<char*>("N");
RtToken		RI_NP;
const RtToken RI_CS = const_cast<char*>("Cs");
const RtToken RI_OS = const_cast<char*>("Os");
const RtToken RI_S = const_cast<char*>("s");
const RtToken RI_T = const_cast<char*>("t");
const RtToken RI_ST	= const_cast<char*>("st");
RtToken RI_BILINEAR, 
		RI_BICUBIC;
RtToken RI_LINEAR, 
		RI_CUBIC;
RtToken RI_PRIMITIVE, 
		RI_INTERSECTION, 
		RI_UNION, 
		RI_DIFFERENCE;
RtToken RI_PERIODIC, 
		RI_NONPERIODIC, 
		RI_CLAMP, 
		RI_BLACK;
RtToken RI_IGNORE, 
		RI_PRINT, 
		RI_ABORT, 
		RI_HANDLER;
RtToken RI_COMMENT, 
		RI_STRUCTURE, 
		RI_VERBATIM;
RtToken RI_IDENTIFIER, 
		RI_NAME, 
		RI_SHADINGGROUP;
RtToken RI_WIDTH, 
		RI_CONSTANTWIDTH;

RtBasis	RiBezierBasis, 
		RiBSplineBasis, 
		RiCatmullRomBasis,
		RiHermiteBasis, 
		RiPowerBasis;

RtInt	RiLastError;

// RI subroutines
RtFloat	RiGaussianFilter(RtFloat x, RtFloat y, RtFloat xwidth, RtFloat ywidth)
{
	return 0.0;
}

RtFloat	RiBoxFilter(RtFloat x, RtFloat y, RtFloat xwidth, RtFloat ywidth)
{
	return 0.0;
}

RtFloat	RiTriangleFilter(RtFloat x, RtFloat y, RtFloat xwidth, RtFloat ywidth)
{
	return 0.0;
}

RtFloat	RiCatmullRomFilter(RtFloat x, RtFloat y, RtFloat xwidth, RtFloat ywidth)
{
	return 0.0;
}

RtFloat	RiSincFilter(RtFloat x, RtFloat y, RtFloat xwidth, RtFloat ywidth)
{
	return 0.0;
}

RtVoid	RiErrorIgnore(RtInt code, RtInt severity, char *msg)
{
}

RtVoid	RiErrorPrint(RtInt code, RtInt severity, char *msg)
{
}

RtVoid	RiErrorAbort(RtInt code, RtInt severity, char *msg)
{
}

RtVoid 	RiProcDelayedReadArchive(RtPointer data, RtFloat detail)
{
}

RtVoid	RiProcRunProgram(RtPointer data, RtFloat detail)
{
}

RtVoid	RiProcDynamicLoad(RtPointer data, RtFloat detail)
{
}

/*
RiGetContext and RiContext have no RIB equivalents
*/
RtContextHandle RiGetContext(void)
{
	return 0;
}

RtVoid	RiContext(RtContextHandle)
{
}

RtToken	RiDeclare(char *name, char *declaration)
{
	return 0;
}


/**
 * RiBegin creates and initializes a new rendering context, setting all graphics state
 * variables to their default values, and makes the new context the active one to which
 * subsequent Ri routines will apply. Any previously active rendering context still exists,
 * but is no longer the active one. The name may be the name of a renderer, to select
 * among various implementations that may be available, or the name of the file to write
 * (in the case of a RIB generator). RI NULL indicates that the default implementation
 * and/or output file should be used.
 * RiBegin and RiEnd have no RIB equivalents. it can be implied that these are implied
 * to occur at the start and end of a RIB file.
 */
RtVoid RiBegin(RtToken name)
{
	std::string str;
	if (name != RI_NULL)
	{
		str = name;
	}
	_renderer.createRenderContext(str);
}


/**
 * RiEnd terminates the active rendering context, including performing any cleanup
 * operations that need to be done. After RiEnd is called, there is no active rendering
 * context until another RiBegin or RiContext is called.
 * All other RenderMan Interface procedures must be called within an active context
 * (the only exceptions are RiErrorHandler, RiOption, and RiContext).
 */
RtVoid RiEnd(void)
{
	_renderer.destroyActiveRenderContext();
}

RtVoid RiFrameBegin(RtInt frame)
{
}

RtVoid RiFrameEnd(void)
{
}

/*
When RiWorldBegin is invoked, all rendering options are frozen and cannot be
changed until the picture is finished. The world-to-camera transformation is set to the
current transformation and the current transformation is reinitialized to the identity. Inside
an RiWorldBegin-RiWorldEnd block, the current transformation is interpreted to
be the object-to-world transformation. After an RiWorldBegin, the interface can accept
geometric primitives that define the scene. (The only other mode in which geometric
primitives may be defined is inside a RiObjectBegin-RiObjectEnd block.) Some
rendering programs may immediately begin rendering geometric primitives as they
are defined, whereas other rendering programs may wait until the entire scene has
been defined.
*/
RtVoid RiWorldBegin(void)
{
	_renderer.activeRenderContext().prepareWorld();
}

/*
RiWorldEnd does not normally return until the rendering program has completed
drawing the image. If the image is to be saved in a file, this is done automatically by
RiWorldEnd.
All lights and retained objects defined inside the RiWorldBegin-RiWorldEnd world
block are removed and their storage reclaimed when RiWorldEnd is called (thus invalidating
their handles).
*/
RtVoid RiWorldEnd(void)
{
	_renderer.activeRenderContext().render();
}

/*
Set the horizontal (xresolution) and vertical (yresolution) resolution (in pixels) of the
image to be rendered. The upper left hand corner of the image has coordinates (0,0)
and the lower right hand corner of the image has coordinates (xresolution, yresolution).
If the resolution is greater than the maximum resolution of the device, the
desired image is clipped to the device boundaries (rather than being shrunk to fit
inside the device). This command also sets the pixel aspect ratio. The pixel aspect
ratio is the ratio of the physical width to the height of a single pixel. The pixel aspect
ratio should normally be set to 1 unless a picture is being computed specifically for a
display device with non-square pixels.
Implicit in this command is the creation of a display viewport with a
viewportaspectratio = (xresolution * pixelaspectratio) / yresolution
The viewport aspect ratio is the ratio of the physical width to the height of the entire
image.
An image of the desired aspect ratio can be specified in a device independent way using
the procedure RiFrameAspectRatio described below. The RiFormat command
should only be used when an image of a specified resolution is needed or an image
file is being created.
If this command is not given, the resolution defaults to that of the display device
being used (see the Displays section, p. 27). Also, if xresolution, yresolution or pixelaspectratio
is specified as a nonpositive value, the resolution defaults to that of the
display device for that particular parameter.
*/
RtVoid RiFormat(RtInt xres, RtInt yres, RtFloat aspect)
{
	_renderer.activeRenderContext().imageResolution(xres, yres, aspect);
}

RtVoid RiFrameAspectRatio(RtFloat aspect)
{
}

RtVoid RiScreenWindow(RtFloat left, RtFloat right, RtFloat bot, RtFloat top)
{
}

RtVoid RiCropWindow(RtFloat xmin, RtFloat xmax, RtFloat ymin, RtFloat ymax)
{
}

/*
The projection determines how camera coordinates are converted to screen coordinates,
using the type of projection and the near/far clipping planes to generate a
projection matrix. It appends this projection matrix to the current transformation matrix
and stores this as the screen transformation, then marks the current coordinate
system as the camera coordinate system and reinitializes the current transformation
matrix to the identity camera transformation. The required types of projection are
”perspective”, ”orthographic”, and RI NULL.
”perspective” builds a projection matrix that does a perspective projection along the
z-axis, using the RiClipping values, so that points on the near clipping plane project
to z = 0 and points on the far clipping plane project to z = 1. ”perspective” takes one
optional parameter, ”fov”, a single RtFloat that indicates he full angle perspective field
of view (in degrees) between screen space coordinates (-1,0) and (1,0) (equivalently
between (0,-1) and (0,1)). The default is 90 degrees.
Note that there is a redundancy in the focal length implied by this procedure and the
one set by RiDepthOfField. The focal length implied by this command is:
focallength = (horizontalscreenwidth / verticalscreenwidth) / tan(fov / 2)
”orthographic” builds a simple orthographic projection that scales z using the RiClipping
values as above. ”orthographic” takes no parameters.
RI NULL uses an identity projection matrix, and simply marks camera space in situations
where the user has generated his own projection matrices himself using RiPerspective
or RiTransform.
This command can also be used to select implementation-specific projections or special
projections written in the Shading Language. If a particular implementation does
not support the special projection specified, it is ignored and an orthographic projection
is used. If RiProjection is not called, the screen transformation defaults to the
identity matrix, so screen space and camera space are identical.
*/
RtVoid RiProjection(RtToken name, ...)
{
	// get parameter list
	va_list ap;
	va_start(ap, name);
	RtToken param = va_arg(ap, RtToken);
	std::string token;
	float fov = 90.;
	if (name == RI_NULL)
		_renderer.activeRenderContext().projection("");
	else if (!strncmp(name, RI_PERSPECTIVE, 11))
	{
		// perspective takes an optional fov parameter
		if (param != RI_NULL)
		{
			token = va_arg(ap, RtToken);
			if (token == RI_FOV)
			{
				fov = va_arg(ap, double);
			}
		}
		_renderer.activeRenderContext().projection(name, fov);
	}
	else
	{
		_renderer.activeRenderContext().projection(name);
	}
	va_end(ap);
}

RtVoid RiProjectionV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

/*
Sets the position of the near and far clipping planes along the direction of view.
near and far must both be positive numbers. near must be greater than or equal
to RI EPSILON and less than far . far must be greater than near and may be equal to
RI INFINITY. These values are used by RiProjection to generate a screen projection
such that depth values are scaled to equal zero at z=near and one at z=far . Notice
that the rendering system will actually clip geometry which lies outside of z=(0,1)
in the screen coordinate system, so non-identity screen transforms may affect which
objects are actually clipped.
For reasons of efficiency, it is generally a good idea to bound the scene tightly with
the near and far clipping planes.
*/
RtVoid RiClipping(RtFloat hither, RtFloat yon)
{
	_renderer.activeRenderContext().clipping(hither, yon);
}

RtVoid RiClippingPlane(RtFloat x, RtFloat y, RtFloat z, RtFloat nx, RtFloat ny, RtFloat nz)
{
}

RtVoid RiShutter(RtFloat min, RtFloat max)
{
}

RtVoid RiPixelVariance(RtFloat variation)
{
}

RtVoid RiPixelSamples(RtFloat xsamples, RtFloat ysamples)
{
}

RtVoid RiPixelFilter(RtFilterFunc filterfunc, RtFloat xwidth, RtFloat ywidth)
{
}

RtVoid RiExposure(RtFloat gain, RtFloat gamma)
{
}

RtVoid RiImager(RtToken name, ...)
{
}

RtVoid RiImagerV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiQuantize(RtToken type, RtInt one, RtInt min, RtInt max, RtFloat ampl)
{
}

/*
Choose a display by name and set the type of output being generated. name is either
the name of a picture file or the name of the framebuffer, depending on type.
The type of display is the display format, output device, or output driver. All implementations
must support the type names ”framebuffer” and ”file”, which indicate
that the renderer should select the default framebuffer or default file format, respectively.
Implementations may support any number of particular formats or devices
(for example, ”tiff” might indicate that a TIFF file should be written), and may allow
the supported formats to be user-extensible in an implementation-specific manner.
The mode indicates what data are to be output in this display stream. All renderers
must support any combination (string concatenation) of ”rgb” for color (usually red,
green and blue intensities unless there are more or less than 3 color samples; see the
next section, Additional options), ”a” for alpha, and ”z” for depth values, in that order.
Renderers may additionally produce “images” consisting of arbitrary data, by using a
mode that is the name of a known geometric quantity or the name of a shader output
variable. Note also that multiple display channels can be specified, by prepending
the + character to the name. For example,
RiDisplay (”out.tif,” ”file,” ”rgba”, RI NULL);
RiDisplay (”+normal.tif,” ”file,” ”N”, RI NULL);
will produce a four-channel image consisting of the filtered color and alpha in out.tif,
and also a second three-channel image file normal.tif consisting of the surface normal
of the nearest surface behind each pixel. (This would, of course, only be useful if
RiQuantize were instructed to output floating point data or otherwise scale the data.)
Display options or device-dependent display modes or functions may be set using
the parameterlist. One such option is required: ”origin”, which takes an array of two
RtInts, sets the x and y position of the upper left hand corner of the image in the
display’s coordinate system; by default the origin is set to (0,0). The default display
device is renderer implementation-specific.
*/
RtVoid RiDisplay(char *name, RtToken type, RtToken mode, ...)
{
}

RtVoid RiDisplayV(char *name, RtToken type, RtToken mode, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiHider(RtToken type, ...)
{
}

RtVoid RiHiderV(RtToken type, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiColorSamples(RtInt n, RtFloat nRGB[], RtFloat RGBn[])
{
}

RtVoid RiRelativeDetail(RtFloat relativedetail)
{
}

RtVoid RiOption(RtToken name, ...)
{
}

RtVoid RiOptionV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiAttributeBegin(void)
{
}

RtVoid RiAttributeEnd(void)
{
}

/*
Set the current color to color. Normally there are three components in the color (red,
green, and blue), but this may be changed with the colorsamples request.
*/
RtVoid RiColor(RtColor color)
{
//	RenderEngine::instance().setCurrentColor(Color3(color[0], color[1], color[2]));
}

RtVoid RiOpacity(RtColor color)
{
}

RtVoid RiTextureCoordinates(RtFloat s1, RtFloat t1, RtFloat s2, RtFloat t2, RtFloat s3, RtFloat t3, RtFloat s4, RtFloat t4)
{
}

/*
shadername is the name of a light source shader. This procedure creates a non-area
light, turns it on, and adds it to the current light source list. An RtLightHandle value
is returned that can be used to turn the light off or on again.
*/
RtLightHandle RiLightSource(RtToken name, ...)
{
	return 0;
}

RtLightHandle RiLightSourceV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[])
{
	return 0;
}

RtLightHandle RiAreaLightSource(RtToken name, ...)
{
	return 0;
}

RtLightHandle RiAreaLightSourceV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[])
{
	return 0;
}

RtVoid RiIlluminate(RtLightHandle light, RtBoolean onoff)
{
}

/*
shadername is the name of a surface shader. This procedure sets the current surface
shader to be shadername. If the surface shader shadername is not defined, some
implementation-dependent default surface shader (but not ”null”) is used.
*/
RtVoid RiSurface(const RtToken name, ...)
{
}

RtVoid RiSurfaceV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiAtmosphere(RtToken name, ...)
{
}

RtVoid RiAtmosphereV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiInterior(RtToken name, ...)
{
}

RtVoid RiInteriorV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiExterior(RtToken name, ...)
{
}

RtVoid RiExteriorV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiShadingRate(RtFloat size)
{
}

RtVoid RiShadingInterpolation(RtToken type)
{
}

RtVoid RiMatte(RtBoolean onoff)
{
}

RtVoid RiBound(RtBound bound)
{
}

RtVoid RiDetail(RtBound bound)
{
}

RtVoid RiDetailRange(RtFloat minvis, RtFloat lowtran, RtFloat uptran, RtFloat maxvis)
{
}

RtVoid RiGeometricApproximation(RtToken type, RtFloat value)
{
}

RtVoid RiOrientation(RtToken orientation)
{
}

RtVoid RiReverseOrientation(void)
{
}

RtVoid RiSides(RtInt sides)
{
}

/*
Set the current transformation to the identity.
*/
RtVoid RiIdentity(void)
{
	_renderer.activeRenderContext().setIdentityTransform();
}

/*
Set the current transformation to the transformation transform.
*/
RtVoid RiTransform(RtMatrix transform)
{
	//Moya::getInstance().getActiveRenderContext().setTransform(transform);
}

RtVoid RiPerspective(RtFloat fov)
{
}

/*
Concatenate a translation onto the current transformation.
*/
RtVoid RiTranslate(RtFloat dx, RtFloat dy, RtFloat dz)
{
	_renderer.activeRenderContext().translate(dx, dy, dz);
}

/*
Concatenate a rotation of angle degrees about the given axis onto the current transformation.
*/
RtVoid RiRotate(RtFloat angle, RtFloat dx, RtFloat dy, RtFloat dz)
{
	_renderer.activeRenderContext().rotate(angle, dx, dy, dz);
}

/*
Concatenate a scaling onto the current transformation.
*/
RtVoid RiScale(RtFloat sx, RtFloat sy, RtFloat sz)
{
	_renderer.activeRenderContext().scale(sx, sy, sz);
}

RtVoid RiSkew(RtFloat angle, RtFloat dx1, RtFloat dy1, RtFloat dz1, RtFloat dx2, RtFloat dy2, RtFloat dz2)
{
}

RtVoid RiDeformation(RtToken name, ...)
{
}

RtVoid RiDeformationV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

/*
Set the current displacement shader to the named shader. shadername is the name of a
displacement shader.
If a particular implementation does not support the Displacements capability, displacement
shaders can only change the normal vectors to generate bump mapping,
and the surface geometry itself is not modified (see Displacement Shaders).
*/
RtVoid RiDisplacement(RtToken name, ...)
{
}

RtVoid RiDisplacementV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiCoordinateSystem(RtToken space)
{
	_renderer.activeRenderContext().saveCoordinateSystem(space);
}

RtVoid RiCoordSysTransform(RtToken space)
{
	_renderer.activeRenderContext().setCoordinateSystem(space);
}

RtPoint * RiTransformPoints(RtToken fromspace, RtToken tospace, RtInt n, RtPoint points[])
{
	return 0;
}

/*
Push and pop the current transformation. Pushing and popping must be properly
nested with respect to the various begin-end constructs.
*/
RtVoid RiTransformBegin(void)
{
	_renderer.activeRenderContext().pushTransform();
}

RtVoid RiTransformEnd(void)
{
	_renderer.activeRenderContext().popTransform();
}

RtVoid RiAttribute(RtToken name, ...)
{
}

RtVoid RiAttributeV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

/*
nvertices is the number of vertices in a single closed planar convex polygon. parameterlist
is a list of token-array pairs where each token is one of the standard geometric primitive
variables or a variable which has been defined with RiDeclare. The parameter
list must include at least position (”P”) information. If a primitive variable is of classss
vertex or varying, the array contains nvertices elements of the type corresponding
to the token. If the variable is uniform or constant, the array contains a single element.
The number of floats associated with each type is given in Table 5.1, Standard
Geometric Primitive Variables.
No checking is done by the RenderMan Interface to ensure that polygons are planar,
convex and nondegenerate. The rendering program will attempt to render invalid
polygons but the results are unpredictable.
*/
RtVoid RiPolygon(RtInt nverts, ...)
{
	/*
		parameterlist is a list of token-array pairs - or -
		a sequence of pairs of arguments, the first being an RtToken and the 
		second being an RtPointer, an untyped pointer to an array of either
		RtFloat, RtString or other values. The list is terminated by the special
		token RI_NULL.
		
		
		tokens are:
			
			Info			Name	Type		Class		Floats
			------------------------------------------------------
			Position		"P"		point		vertex		3
							"Pz"	float		vertex		1
							"Pw"	h			vertex		4
			Normal			"N"		normal		varying		3
			Color			"Cs"	color		varying		(3)
			Opacity			"Os"	color		varying		(3)
			Texture Coords	"s"		float		varying		1
							"t"		float		varying		1
							"st"	2 float		varying		2
		crorresponding tokens:
			RI_P, RI_PZ, RI_PW, RI_N, RI_CS, RI_OS, RI_S, RI_T, RI_ST

		usage:
			RtPoint points[4] = { 0.0, 1.0, 0.0,	0.0, 1.0, 1.0,
								  0.0, 0.0, 1.0,	0.0, 0.0, 0.0 };
			RiPolygon(4, RI_P, (RtPointer)points, RI_NULL);

		gourad shaded:
			RtColor colors[4];
			RiPolygon(4, "P", (RtPointer)points, "Cs", (RtPointer)colors, RI_NULL);

		phong shaded:
			RtPoint normals[4];
			RiPolygon(4, "P", (RtPointer)points, "N", (RtPointer)normals, RI_NULL);
	*/
	va_list ap;
	
	va_start(ap, nverts);

	RtToken name = va_arg(ap, RtToken);
	std::string token;
	boost::shared_ptr<Polygon> poly(new Polygon);
	while (name != RI_NULL)
	{
		token = name;
		RtPointer p = va_arg(ap, RtPointer);
		if (token == RI_P)
		{
			// p contains RtPoint points[nverts]
			Vertex v;
			RtPoint *points = (RtPoint*)p;
			for (int i = 0; i < nverts; i++)
			{
				v.point(glm::vec3(points[i][0], points[i][1], points[i][2]));
				poly->addVertex(v);
			}
		}
		else if (token == RI_PZ)
		{
		}
		else if (token == RI_PW)
		{
		}
		else if (token == RI_N)
		{
			// p contains RtPoint normals[nverts]
		}
		else if (token == RI_CS)
		{
			// p contains RtColor colors[nverts]
		}
		else if (token == RI_OS)
		{
		}
		else if (token == RI_S)
		{
		}
		else if (token == RI_T)
		{
		}
		else if (token == RI_ST)
		{
		}
		// else error

		name = va_arg(ap, RtToken);
	}
	va_end(ap);
	
	// add poly to renderer
	_renderer.activeRenderContext().addPolygon(poly);
}

RtVoid RiPolygonV(RtInt nverts, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiGeneralPolygon(RtInt nloops, RtInt nverts[], ...)
{
}

RtVoid RiGeneralPolygonV(RtInt nloops, RtInt nverts[], RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiPointsPolygons(RtInt npolys, RtInt nverts[], RtInt verts[], ...)
{
}

RtVoid RiPointsPolygonsV(RtInt npolys, RtInt nverts[], RtInt verts[], RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiPointsGeneralPolygons(RtInt npolys, RtInt nloops[], RtInt nverts[], RtInt verts[], ...)
{
}

RtVoid RiPointsGeneralPolygonsV(RtInt npolys, RtInt nloops[], RtInt nverts[], RtInt verts[], RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiBasis(RtBasis ubasis, RtInt ustep, RtBasis vbasis, RtInt vstep)
{
}

RtVoid RiPatch(RtToken type, ...)
{
}

RtVoid RiPatchV(RtToken type, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiPatchMesh(RtToken type, RtInt nu, RtToken uwrap, RtInt nv, RtToken vwrap, ...)
{
}

RtVoid RiPatchMeshV(RtToken type, RtInt nu, RtToken uwrap, RtInt nv, RtToken vwrap, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiNuPatch(RtInt nu, RtInt uorder, RtFloat uknot[], RtFloat umin, RtFloat umax, RtInt nv, RtInt vorder, RtFloat vknot[], RtFloat vmin, RtFloat vmax, ...)
{
}

RtVoid RiNuPatchV(RtInt nu, RtInt uorder, RtFloat uknot[], RtFloat umin, RtFloat umax, RtInt nv, RtInt vorder, RtFloat vknot[], RtFloat vmin, RtFloat vmax, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiTrimCurve(RtInt nloops, RtInt ncurves[], RtInt order[], RtFloat knot[], RtFloat min[], RtFloat max[], RtInt n[], RtFloat u[], RtFloat v[], RtFloat w[])
{
}

/*
Requests a sphere defined by the following equations:

*/
RtVoid RiSphere(RtFloat radius, RtFloat zmin, RtFloat zmax, RtFloat tmax, ...)
{
}

RtVoid RiSphereV(RtFloat radius, RtFloat zmin, RtFloat zmax, RtFloat tmax, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiCone(RtFloat height, RtFloat radius, RtFloat tmax, ...)
{
}

RtVoid RiConeV(RtFloat height, RtFloat radius, RtFloat tmax, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiCylinder(RtFloat radius, RtFloat zmin, RtFloat zmax, RtFloat tmax, ...)
{
}

RtVoid RiCylinderV(RtFloat radius, RtFloat zmin, RtFloat zmax, RtFloat tmax, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiHyperboloid(RtPoint point1, RtPoint point2, RtFloat tmax, ...)
{
}

RtVoid RiHyperboloidV(RtPoint point1, RtPoint point2, RtFloat tmax, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiParaboloid(RtFloat rmax, RtFloat zmin, RtFloat zmax, RtFloat tmax, ...)
{
}

RtVoid RiParaboloidV(RtFloat rmax, RtFloat zmin, RtFloat zmax, RtFloat tmax, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiDisk(RtFloat height, RtFloat radius, RtFloat tmax, ...)
{
}

RtVoid RiDiskV(RtFloat height, RtFloat radius, RtFloat tmax, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiTorus(RtFloat majrad, RtFloat minrad, RtFloat phimin, RtFloat phimax, RtFloat tmax, ...)
{
}

RtVoid RiTorusV(RtFloat majrad, RtFloat minrad, RtFloat phimin, RtFloat phimax, RtFloat tmax, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiBlobby(RtInt nleaf, RtInt ncode, RtInt code[], RtInt nflt, RtFloat flt[], RtInt nstr, RtToken str[], ...)
{
}

RtVoid RiBlobbyV(RtInt nleaf, RtInt ncode, RtInt code[], RtInt nflt, RtFloat flt[], RtInt nstr, RtToken str[], RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiCurves(RtToken type, RtInt nvcurves, RtInt nvertices[], RtToken wrap, ...)
{
}

RtVoid RiCurvesV(RtToken type, RtInt nvcurves, RtInt nvertices[], RtToken wrap, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiPoints(RtInt nverts, ...)
{
}

RtVoid RiPointsV(RtInt nverts, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiSubdivisionMesh(RtToken mask, RtInt nf, RtInt nverts[], RtInt verts[], RtInt ntags, RtToken tags[], RtInt numargs[], RtInt intargs[], RtFloat floatargs[], ...)
{
}

RtVoid RiSubdivisionMeshV(RtToken mask, RtInt nf, RtInt nverts[], RtInt verts[], RtInt ntags, RtToken tags[], RtInt numargs[], RtInt intargs[], RtFloat floatargs[], RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid	RiProcedural(RtPointer data, RtBound bound, RtVoid (*subdivfunc)(RtPointer, RtFloat), RtVoid (*freefunc)(RtPointer))
{
}

RtVoid RiGeometry(RtToken type, ...)
{
}

RtVoid RiGeometryV(RtToken type, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid	RiSolidBegin(RtToken operation)
{
}

RtVoid RiSolidEnd(void)
{
}

RtObjectHandle RiObjectBegin(void)
{
	return 0;
}

RtVoid	RiObjectEnd(void)
{
}

RtVoid RiObjectInstance(RtObjectHandle handle)
{
}

RtVoid RiMotionBegin(RtInt n, ...)
{
}

RtVoid RiMotionBeginV(RtInt n, RtInt n2, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiMotionEnd(void)
{
}

RtVoid RiMakeTexture(char *pic, char *tex, RtToken swrap, RtToken twrap, RtFilterFunc filterfunc, RtFloat swidth, RtFloat twidth, ...)
{
}

RtVoid RiMakeTextureV(char *pic, char *tex, RtToken swrap, RtToken twrap, RtFilterFunc filterfunc, RtFloat swidth, RtFloat twidth, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiMakeBump(char *pic, char *tex, RtToken swrap, RtToken twrap, RtFilterFunc filterfunc, RtFloat swidth, RtFloat twidth, ...)
{
}

RtVoid RiMakeBumpV(char *pic, char *tex, RtToken swrap, RtToken twrap, RtFilterFunc filterfunc, RtFloat swidth, RtFloat twidth, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiMakeLatLongEnvironment(char *pic, char *tex, RtFilterFunc filterfunc, RtFloat swidth, RtFloat twidth, ...)
{
}

RtVoid RiMakeLatLongEnvironmentV(char *pic, char *tex, RtFilterFunc filterfunc, RtFloat swidth, RtFloat twidth, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiMakeCubeFaceEnvironment(char *px, char *nx, char *py, char *ny, char *pz, char *nz, char *tex, RtFloat fov, RtFilterFunc filterfunc, RtFloat swidth, RtFloat ywidth, ...)
{
}

RtVoid RiMakeCubeFaceEnvironmentV(char *px, char *nx, char *py, char *ny, char *pz, char *nz, char *tex, RtFloat fov, RtFilterFunc filterfunc, RtFloat swidth, RtFloat ywidth, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiMakeShadow(char *pic, char *tex, ...)
{
}

RtVoid RiMakeShadowV(char *pic, char *tex, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiArchiveRecord(RtToken type, char *format, ...)
{
}

RtVoid RiReadArchive(RtToken name, RtArchiveCallback callback, ...)
{
}

RtVoid RiReadArchiveV(RtToken name, RtArchiveCallback callback, RtInt n, RtToken tokens[], RtPointer parms[])
{
}

RtVoid RiErrorHandler(RtErrorHandler handler)
{
}
