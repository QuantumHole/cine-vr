// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "node_css.h"
#include <sstream>
#include <regex>
#include <algorithm>
#include <cctype>
#include <locale>

// Trim from both ends
static std::string trim(const std::string& t)
{
	std::string s(t);

	// Trim from the end (in place)
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base(), s.end());

	// Trim from the start (in place)
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	}));

	return s;
}

NodeCSS::NodeCSS(void) :
	m_selector(""),
	m_attributes(),
	m_children()
{
}

NodeCSS::NodeCSS(const std::string& name, const std::string& attributes) :
	m_selector(name),
	m_attributes(),
	m_children()
{
	parseAttributes(attributes);
}

std::string NodeCSS::print(const size_t level) const
{
	std::stringstream s;
	std::string pad(level, '\t');

	s << pad << "selector: " << m_selector << std::endl;
	for (std::map<std::string, std::string>::const_iterator attr = m_attributes.begin(); attr != m_attributes.end(); ++attr)
	{
		s << pad << "\t" << attr->first << " = " << attr->second << std::endl;
	}

	for (std::vector<NodeCSS>::const_iterator node = m_children.begin(); node != m_children.end(); ++node)
	{
		s << node->print(level + 1);
	}
	return s.str();
}

void NodeCSS::parseAttributes(const std::string& attrString)
{
	std::regex attrRegex(R"(([^:;]+):([^;]+);?)");
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

void NodeCSS::parse(const std::string& content, std::string::const_iterator& start)
{
	// Alle CSS‑Blöcke (Selector { … }) finden
	const std::regex tagRegex(R"(([^{}]+)\{([^{}]+)\})");
	std::smatch match;

	while (std::regex_search(start, content.cend(), match, tagRegex))
	{
		const std::string selectorName = trim(match[1].str());
		const std::string attrString = match[2].str();

		// Create the child node
		NodeCSS child(selectorName, attrString);
		m_children.push_back(child);

		// Move iterator to the character right after the complete match
		start = match[0].second;
	}
}

const std::string& NodeCSS::selector(void) const
{
	return m_selector;
}

const std::map<std::string, std::string>& NodeCSS::attributes(void) const
{
	return m_attributes;
}

const std::vector<NodeCSS>& NodeCSS::children(void) const
{
	return m_children;
}

void NodeCSS::parse(const std::string& content)
{
	std::string::const_iterator start = content.cbegin();

	parse(content, start);
}

std::string NodeCSS::print(void) const
{
	return print(0);
}
