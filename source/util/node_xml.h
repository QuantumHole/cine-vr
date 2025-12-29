// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NODE_XML_H
#define NODE_XML_H

#include <string>
#include <map>
#include <vector>

class NodeXML
{
	private:
		std::string m_tag;
		std::string m_content;
		std::map<std::string, std::string> m_attributes;
		std::vector<NodeXML> m_children;

		explicit NodeXML(const std::string& name, const std::string& attributes);

		std::string print(const size_t level) const;
		void parseAttributes(const std::string& attrString);
		void parse(const std::string& content, std::string::const_iterator& start);

	public:
		explicit NodeXML(void);
		const std::string& tag(void) const;
		const std::string& content(void) const;
		const std::map<std::string, std::string>& attributes(void) const;
		const std::vector<NodeXML>& children(void) const;
		void parse(const std::string& content);
		std::string print(void) const;
};

#endif
