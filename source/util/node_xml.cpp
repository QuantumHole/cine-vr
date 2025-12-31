// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "node_xml.h"
#include "util/string_tools.h"
#include <sstream>
#include <regex>
#include <cctype>
#include <locale>

NodeXML::NodeXML(void) :
	m_tag(""),
	m_content(""),
	m_attributes(),
	m_children()
{
}

NodeXML::NodeXML(const std::string& name, const std::string& attributes) :
	m_tag(name),
	m_content(""),
	m_attributes(),
	m_children()
{
	parseAttributes(attributes);
}

std::string NodeXML::print(const size_t level) const
{
	std::stringstream s;
	std::string pad(level, '\t');

	s << pad << "tag: " << m_tag << std::endl;
	for (std::map<std::string, std::string>::const_iterator attr = m_attributes.begin(); attr != m_attributes.end(); ++attr)
	{
		s << pad << "\t" << attr->first << " = " << attr->second << std::endl;
	}

	if (!m_content.empty())
	{
		s << pad << "\t" << "content: " << m_content << std::endl;
	}
	for (std::vector<NodeXML>::const_iterator node = m_children.begin(); node != m_children.end(); ++node)
	{
		s << node->print(level + 1);
	}
	return s.str();
}

void NodeXML::parseAttributes(const std::string& attrString)
{
	std::regex attrRegex(R"(([\w:-]+)=["']([^"']*)["'])");
	std::smatch match;
	std::string::const_iterator iter = attrString.cbegin();

	while (std::regex_search(iter, attrString.cend(), match, attrRegex))
	{
		std::string key = trim(match[1].str());
		std::string value = trim(match[2].str());
		m_attributes[key] = value;
		iter = match[0].second; // Update search position
	}
}

void NodeXML::parse(const std::string& content, std::string::const_iterator& start)
{
	const std::regex tagRegex(R"(<\/?([a-zA-Z_][\w:-]*)\s*([^>]*)\/?>)");   // opening, closing or self‑closing
	std::smatch match;

	while (std::regex_search(start, content.cend(), match, tagRegex))
	{
		const bool closing = (match[0].str().compare(0, 2, "</") == 0);
		const bool selfclosing = (match[0].str().size() >= 2) &&
		                         (match[0].str().compare(match[0].str().size() - 2, 2, "/>") == 0);
		const std::string tagName = trim(match[1].str());
		const std::string attrString = match[2].str();

		// Position *after* the current tag
		const size_t afterTagPos = static_cast<size_t>(match[0].second - content.begin());

		// ---- 1. Closing tag -------------------------------------------------
		if (closing)
		{
			if (tagName != m_tag)
			{
				throw std::runtime_error("tag mismatch: " + tagName + " / " + m_tag);
			}
			// Nothing more to read for this node – return to caller
			start = match[0].second;
			return;
		}

		// ---- 2. Opening (or self‑closing) tag -------------------------------
		// Create the child node first
		NodeXML child(tagName, attrString);

		// Move iterator to the character right after the opening tag
		start = match[0].second;

		// If the tag is not self‑closing, recurse to fill its children/content
		if (!selfclosing)
		{
			child.parse(content, start);
		}

		// ---- 3. Capture text that belongs to *this* node --------------------
		// Text is the slice from the end of the opening tag up to the next '<'
		size_t nextTagPos = content.find('<', afterTagPos);

		if ((nextTagPos != std::string::npos) && (nextTagPos > afterTagPos))
		{
			std::string rawText = content.substr(afterTagPos, nextTagPos - afterTagPos);
			child.m_content = trim(rawText);
		}

		// ---- 4. Store the child ------------------------------------------------
		m_children.push_back(child);
	}
}

const std::string& NodeXML::tag(void) const
{
	return m_tag;
}

const std::string& NodeXML::content(void) const
{
	return m_content;
}

const std::map<std::string, std::string>& NodeXML::attributes(void) const
{
	return m_attributes;
}

const std::vector<NodeXML>& NodeXML::children(void) const
{
	return m_children;
}

void NodeXML::parse(const std::string& content)
{
	std::string::const_iterator start = content.cbegin();

	parse(content, start);
}

std::string NodeXML::print(void) const
{
	return print(0);
}
