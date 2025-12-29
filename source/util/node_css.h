// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NODE_CSS_H
#define NODE_CSS_H

#include <string>
#include <map>
#include <vector>

class NodeCSS
{
	private:
		std::string m_selector;
		std::map<std::string, std::string> m_attributes;
		std::vector<NodeCSS> m_children;

		explicit NodeCSS(const std::string& name, const std::string& attributes);

		std::string print(const size_t level) const;
		void parseAttributes(const std::string& attrString);
		void parse(const std::string& content, std::string::const_iterator& start);

	public:
		explicit NodeCSS(void);
		const std::string& selector(void) const;
		const std::map<std::string, std::string>& attributes(void) const;
		const std::vector<NodeCSS>& children(void) const;
		void parse(const std::string& content);
		std::string print(void) const;
};

#endif
