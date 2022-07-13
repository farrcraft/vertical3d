/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

/**
 * Load file assets
 */
class AssetLoader
{
	public:
		/**
		 * Default constructor
		 *
		 * @param string path base asset directory path
		 */
		AssetLoader(const std::string & path);

		/**
		 * Load an asset from a file into a string
		 *
		 * @param string name of file in the asset directory
		 *
		 * @return string content of the asset file
		 */
		std::string load(const std::string & filename);

		/**
		 * Get the asset directory path
		 *
		 * @return string full asset directory path
		 */
		std::string path() const;

	private:
		std::string path_;
};
