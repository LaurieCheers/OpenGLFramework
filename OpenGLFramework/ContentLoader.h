#pragma once

class ContentLoader
{
public:
	GLuint LoadShaders(const char * vertex_file_path, const char * fragment_file_path);
	GLuint LoadBMP(const char * imagepath);
};