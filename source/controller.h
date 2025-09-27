#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "opengl/shape.h"

class Controller
{
	private:
		Shape m_body;
		Shape m_line;

		void init_line(void);
		void init_point(void);

	public:
		explicit Controller(void);
		Controller(const Controller& c);
		Controller& operator=(const Controller& c);

		void init(void);
		void set_transform(const glm::mat4& pose);
		void draw(void) const;
};

#endif
