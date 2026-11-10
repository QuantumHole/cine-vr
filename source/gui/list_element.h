#ifndef LIST_ELEMENT_H
#define LIST_ELEMENT_H

#include "database/thumbnail_generator.h"
#include <stddef.h>
#include <string>


class ListElement
{
    private:

    public:
        /** constructor. */
        explicit ListElement(void);

        size_t create_thumbnail_label(const std::string& text, const bool align_right, const bool top_half) const;
        size_t create_text_element(const std::string& text, const size_t indent_level);
        size_t create_video_element(
            const std::string& title,
            const std::string& subtitle,
            const Thumbnail& background) const;
};

#endif
