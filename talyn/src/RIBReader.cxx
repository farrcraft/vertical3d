/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "RIBReader.h"

#include <boost/lexical_cast.hpp>

#include <iostream>

using namespace Talyn;

/*
##RenderMan RIB-Structure 1.1
##Scene Bouncing Ball
##Creator /usr/ucb/vi
##CreationDate 12:30pm 8/24/89
##For RenderMan Jones
##Frames 2
##Shaders PIXARmarble, PIXARwood, MyUserShader
##CapabilitiesNeeded ShadingLanguage Displacements
version 3.03
Declare "d" "uniform point"
Declare "squish" "uniform float"
Option "limits" "bucketsize" [6 6]  #renderer specific
Option "limits" "gridsize" [18]  #renderer specific
Format 1024 768 1  #mandatory resolution
Projection "perspective"
Clipping 10 1000.0
FrameBegin 1
##Shaders MyUserShader, PIXARmarble, PIXARwood
##CameraOrientation 10.0 10.0 10.0 0.0 0.0 0.0
Transform  [.707107  -.408248  -.57735 0
            0  .816497  -.57735  0
            -.707107  -.408248  -.57735  0
            0  0  17.3205  1 ]
WorldBegin
AttributeBegin
Attribute "identifier" "name" "myball"
Displacement "MyUserShader" "squish" 5
AttributeBegin
Attribute "identifier" "shadinggroup" ["tophalf"]
Surface "PIXARmarble"
Sphere .5 0 .5 360
AttributeEnd
AttributeBegin
Attribute "identifier" "shadinggroup" ["bothalf"]
Surface "plastic"
Sphere .5 -.5 0. 360
AttributeEnd
AttributeEnd
AttributeBegin
Attribute "identifier" "name" ["floor"]
Surface "PIXARwood" "roughness" [.3] "d" [1]
# geometry for floor
Polygon "P" 
[-100. 0. -100.  -100. 0. 100.  100. 0. 100.  10.0 0. -100.]
AttributeEnd
WorldEnd
FrameEnd
FrameBegin 2
##Shaders PIXARwood, PIXARmarble
##CameraOrientation 10.0 20.0 10.0 0.0 0.0 0.0
Transform [.707107  -.57735  -.408248  0
           0   .57735
           -.815447 0
           -.707107  -.57735  -.408248  0
           0  0  24.4949 1 ]
WorldBegin
AttributeBegin
Attribute "identifier" "name" ["myball"]
AttributeBegin
Attribute "identifier" "shadinggroup" ["tophalf"]
Surface "PIXARmarble"
ShadingRate .1
Sphere .5 0 .5 360
AttributeEnd
AttributeBegin
Attribute "identifier" "shadinggroup" ["bothalf"]
Surface "plastic"
Sphere .5 -.5 0 360
AttributeEnd
AttributeEnd
AttributeBegin
Attribute "identifier" "name" ["floor"]
Surface "PIXARwood" "roughness" [.3] "d" [1]
# geometry for floor
AttributeEnd
WorldEnd
FrameEnd
*/

RIBReader::RIBReader(const boost::shared_ptr<RenderContext> & rc) : rc_(rc)
{
}

std::string RIBReader::readToken()
{
    std::string token;
    file_ >> token;
    return token;
}

void RIBReader::parseToken(const std::string & token)
{

    if (token == "WorldBegin")
    {
    }
    else if (token == "WorldEnd")
    {
    }
    else if (token == "FrameBegin")
    {
    }
    else if (token == "FrameEnd")
    {
    }
    else if (token == "AttributeBegin")
    {
    }
    else if (token == "Attribute")
    {
    }
    else if (token == "AttributeEnd")
    {
    }
    else if (token == "Transform")
    {
    }
    else if (token == "Sphere")
    {
    }
    else if (token == "Surface")
    {
    }
    else if (token == "Polygon")
    {
    }
    else if (token == "Displacement")
    {
    }
    else if (token == "version")
    {
    }
    else if (token == "Option")
    {
    }
    else if (token == "Clipping")
    {
    }
    else if (token == "Format")
    {
        std::string xres, yres, aspect;
        xres = readToken();
        yres = readToken();
        aspect = readToken();
        unsigned int width = boost::lexical_cast<unsigned int>(xres);
        unsigned int height = boost::lexical_cast<unsigned int>(yres);
        rc_->format(width, height);
    }
    else if (token == "Projection")
    {
    }
    else if (token == "ShadingRate")
    {
    }
    else if (token == "Declare")
    {
    }
    else if (token == "##RenderMan")
    {
    }
    else if (token == "##Scene")
    {
    }
    else if (token == "##Creator")
    {
    }
    else if (token == "##CreationDate")
    {
    }
    else if (token == "##For")
    {
    }
    else if (token == "##Frames")
    {
    }
    else if (token == "##Shaders")
    {
    }
    else if (token == "##CapabilitiesNeeded")
    {
    }
    else if (token == "##CameraOrientation")
    {
    }
}

bool RIBReader::read(const std::string &filename)
{
    int length = 0;
    file_.exceptions ( /*std::ifstream::eofbit |  std::ifstream::failbit | */std::ifstream::badbit );
    try
    {
        file_.open(filename.c_str());
        if (!file_)
        {
            std::cout << "Error opening file - " << filename << std::endl;
            return false;
        }
        std::string token;
        while (!file_.eof())
        {
            // read a token from the stream
            token = readToken();
            std::cout << "read token: " << token << std::endl;
            // parse the token
            parseToken(token);
        }
    }
    catch (std::ifstream::failure e)
    {
        std::cout << "Exception opening/reading file '" << filename << "' - " << e.what() << std::endl;
        return false;
    }

    file_.close();

    return true;
}
